// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#pragma once

internal T_Result t_codec_pe_decode(T_Context *ctx, T_Artifact *artifact, MD_Node **semantic_tree_out);
internal B32 t_codec_coff_section_name(String8 string_table, COFF_SectionHeader *header, String8 *name_out);
internal T_Result t_codec_run_validate(T_ParseContext *ctx, MD_Node *arguments);
internal T_Result t_codec_run_execute(T_Context *ctx, MD_Node *arguments);
internal T_Result t_codec_clang_validate(T_ParseContext *ctx, MD_Node *arguments);
internal T_Result t_codec_clang_execute(T_Context *ctx, MD_Node *arguments);
internal T_Result t_codec_expect_pe_validate(T_ParseContext *ctx, MD_Node *arguments);
internal T_Result t_codec_expect_pe_execute(T_Context *ctx, MD_Node *arguments);
internal T_Result t_codec_pdb_decode(T_Context *ctx, String8 data, MD_Node **semantic_tree_out);
internal T_Result t_codec_expect_pdb_validate(T_ParseContext *ctx, MD_Node *arguments);
internal T_Result t_codec_expect_pdb_execute(T_Context *ctx, MD_Node *arguments);
internal T_Result t_codec_expect_coff_validate(T_ParseContext *ctx, MD_Node *arguments);
internal T_Result t_codec_expect_coff_execute(T_Context *ctx, MD_Node *arguments);
internal T_Result t_codec_expect_file_validate(T_ParseContext *ctx, MD_Node *arguments);
internal T_Result t_codec_expect_file_execute(T_Context *ctx, MD_Node *arguments);
internal T_Result t_codec_expect_pe_word_validate(T_ParseContext *ctx, MD_Node *arguments);
internal T_Result t_codec_expect_pe_word_execute(T_Context *ctx, MD_Node *arguments);
internal T_Result t_codec_expect_pe_bytes_validate(T_ParseContext *ctx, MD_Node *arguments);
internal T_Result t_codec_expect_pe_bytes_execute(T_Context *ctx, MD_Node *arguments);

global T_SuiteSpec t_codec_script_suite;
