// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

typedef struct LNK_Directive
{
  struct LNK_Directive *next;
  String8               id;
  String8List           value_list;
} LNK_Directive;

typedef struct LNK_DirectiveList
{
  U64            count;
  LNK_Directive *first;
  LNK_Directive *last;
} LNK_DirectiveList;

typedef struct LNK_ExportParse
{
  struct LNK_ExportParse *next;
  String8                 name;
  String8                 alias;
  String8                 type;
} LNK_ExportParse;

typedef struct LNK_ExportParseList
{
  U64              count;
  LNK_ExportParse *first;
  LNK_ExportParse *last;
} LNK_ExportParseList;

typedef struct LNK_MergeDirective
{
  String8 src;
  String8 dst;
} LNK_MergeDirective;

typedef struct LNK_MergeDirectiveNode
{
  struct LNK_MergeDirectiveNode *next;
  LNK_MergeDirective             data;
} LNK_MergeDirectiveNode;

typedef struct LNK_MergeDirectiveList
{
  U64                     count;
  LNK_MergeDirectiveNode *first;
  LNK_MergeDirectiveNode *last;
} LNK_MergeDirectiveList;

typedef enum
{
  LNK_Directive_Null,
  LNK_Directive_DefaultLib,
  LNK_Directive_Export,
  LNK_Directive_Include,
  LNK_Directive_ManifestDependency,
  LNK_Directive_Merge,
  LNK_Directive_Section,
  LNK_Directive_AlternateName,
  LNK_Directive_GuardSym,
  LNK_Directive_DisallowLib,
  LNK_Directive_FailIfMismatch,
  LNK_Directive_EditAndContinue,
  LNK_Directive_ThrowingNew,
  LNK_Directive_Count
} LNK_DirectiveKind;

typedef struct LNK_DirectiveInfo
{
  LNK_DirectiveList v[LNK_Directive_Count];
} LNK_DirectiveInfo;

////////////////////////////////

internal void lnk_alt_name_list_concat_in_place(LNK_AltNameList *list, LNK_AltNameList *to_concat);

internal LNK_MergeDirectiveNode * lnk_merge_directive_list_push(Arena *arena, LNK_MergeDirectiveList *list, LNK_MergeDirective data);

////////////////////////////////

internal void        lnk_parse_directives(Arena *arena, LNK_DirectiveInfo *directive_info, String8 buffer, String8 obj_path);
internal String8List lnk_parse_default_lib_directive(Arena *arena, LNK_DirectiveList *dir_list);
internal B32         lnk_parse_merge_directive(String8 directive, LNK_MergeDirective *out);


