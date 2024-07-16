// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////
//~ rjf: Includes

//- rjf: headers
#include "metagen/metagen_base/metagen_base_inc.h"
#include "metagen/metagen_os/metagen_os_inc.h"
#include "mdesk/mdesk.h"
#include "metagen.h"

//- rjf: impls
#include "metagen/metagen_base/metagen_base_inc.c"
#include "metagen/metagen_os/metagen_os_inc.c"
#include "mdesk/mdesk.c"
#include "metagen.c"

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmdline)
{
  //////////////////////////////
  //- rjf: set up state
  //
  MG_MsgList msgs = {0};
  mg_arena = arena_alloc(.reserve_size = GB(64), .commit_size = MB(64));
  mg_state = push_array(mg_arena, MG_State, 1);
  mg_state->slots_count = 256;
  mg_state->slots = push_array(mg_arena, MG_LayerSlot, mg_state->slots_count);
  
  //////////////////////////////
  //- rjf: extract paths
  //
  String8 build_dir_path   = os_get_process_info()->binary_path;
  String8 project_dir_path = str8_chop_last_slash(build_dir_path);
  String8 code_dir_path    = push_str8f(mg_arena, "%S/src", project_dir_path);
  
  //////////////////////////////
  //- rjf: search code directories for all files to consider
  //
  String8List file_paths = {0};
  DeferLoop(printf("searching %.*s...", str8_varg(code_dir_path)), printf(" %i files found\n", (int)file_paths.node_count))
  {
    typedef struct Task Task;
    struct Task
    {
      Task *next;
      String8 path;
    };
    Task start_task = {0, code_dir_path};
    Task *first_task = &start_task;
    Task *last_task = &start_task;
    for(Task *task = first_task; task != 0; task = task->next)
    {
      OS_FileIter *it = os_file_iter_begin(mg_arena, task->path, 0);
      for(OS_FileInfo info = {0}; os_file_iter_next(mg_arena, it, &info);)
      {
        String8 file_path = push_str8f(mg_arena, "%S/%S", task->path, info.name);
        if(info.props.flags & FilePropertyFlag_IsFolder)
        {
          Task *next_task = push_array(mg_arena, Task, 1);
          SLLQueuePush(first_task, last_task, next_task);
          next_task->path = file_path;
        }
        else
        {
          str8_list_push(mg_arena, &file_paths, file_path);
        }
      }
      os_file_iter_end(it);
    }
  }
  
  //////////////////////////////
  //- rjf: parse all metadesk files
  //
  MG_FileParseList parses = {0};
  DeferLoop(printf("parsing metadesk..."), printf(" %i metadesk files parsed\n", (int)parses.count))
  {
    for(String8Node *n = file_paths.first; n != 0; n = n->next)
    {
      String8 file_path = n->string;
      String8 file_ext = str8_skip_last_dot(file_path);
      if(str8_match(file_ext, str8_lit("mdesk"), 0))
      {
        String8 data = os_data_from_file_path(mg_arena, file_path);
        MD_TokenizeResult tokenize = md_tokenize_from_text(mg_arena, data);
        MD_ParseResult parse = md_parse_from_text_tokens(mg_arena, file_path, data, tokenize.tokens);
        for(MD_Msg *m = parse.msgs.first; m != 0; m = m->next)
        {
          TxtPt pt = mg_txt_pt_from_string_off(data, m->node->src_offset);
          String8 msg_kind_string = {0};
          switch(m->kind)
          {
            default:{}break;
            case MD_MsgKind_Note:        {msg_kind_string = str8_lit("note");}break;
            case MD_MsgKind_Warning:     {msg_kind_string = str8_lit("warning");}break;
            case MD_MsgKind_Error:       {msg_kind_string = str8_lit("error");}break;
            case MD_MsgKind_FatalError:  {msg_kind_string = str8_lit("fatal error");}break;
          }
          String8 location = push_str8f(mg_arena, "%S:%I64d:%I64d", file_path, pt.line, pt.column);
          MG_Msg dst_m = {location, msg_kind_string, m->string};
          mg_msg_list_push(mg_arena, &msgs, &dst_m);
        }
        MG_FileParseNode *parse_n = push_array(mg_arena, MG_FileParseNode, 1);
        SLLQueuePush(parses.first, parses.last, parse_n);
        parse_n->v.root = parse.root;
        parses.count += 1;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: gather tables
  //
  MG_Map table_grid_map = mg_push_map(mg_arena, 1024);
  MG_Map table_col_map = mg_push_map(mg_arena, 1024);
  U64 table_count = 0;
  DeferLoop(printf("gathering tables..."), printf(" %i tables found\n", (int)table_count))
  {
    for(MG_FileParseNode *n = parses.first; n != 0; n = n->next)
    {
      MD_Node *file = n->v.root;
      for(MD_EachNode(node, file->first))
      {
        MD_Node *table_tag = md_tag_from_string(node, str8_lit("table"), 0);
        if(!md_node_is_nil(table_tag))
        {
          MG_NodeGrid *table = push_array(mg_arena, MG_NodeGrid, 1);
          MG_ColumnDescArray *col_descs = push_array(mg_arena, MG_ColumnDescArray, 1);
          *table = mg_node_grid_make_from_node(mg_arena, node);
          *col_descs = mg_column_desc_array_from_tag(mg_arena, table_tag);
          mg_map_insert_ptr(mg_arena, &table_grid_map, node->string, table);
          mg_map_insert_ptr(mg_arena, &table_col_map, node->string, col_descs);
          table_count += 1;
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: gather layer options
  //
  for(MG_FileParseNode *n = parses.first; n != 0; n = n->next)
  {
    MD_Node *file = n->v.root;
    String8 layer_key = mg_layer_key_from_path(file->string);
    MG_Layer *layer = mg_layer_from_key(layer_key);
    for(MD_EachNode(node, file->first))
    {
      if(md_node_has_tag(node, str8_lit("option"), 0))
      {
        if(str8_match(node->string, str8_lit("library"), 0))
        {
          layer->is_library = 1;
        }
      }
      if(md_node_has_tag(node, str8_lit("gen_folder"), 0))
      {
        layer->gen_folder_name = node->string;
      }
      if(md_node_has_tag(node, str8_lit("h_name"), 0))
      {
        layer->h_name_override = node->string;
      }
      if(md_node_has_tag(node, str8_lit("c_name"), 0))
      {
        layer->c_name_override = node->string;
      }
      if(md_node_has_tag(node, str8_lit("h_header"), 0))
      {
        String8List gen_strings = mg_string_list_from_table_gen(mg_arena, table_grid_map, table_col_map, str8_lit(""), node);
        for(String8Node *n = gen_strings.first; n != 0; n = n->next)
        {
          str8_list_push(mg_arena, &layer->h_header, n->string);
          str8_list_push(mg_arena, &layer->h_header, str8_lit("\n"));
        }
      }
      if(md_node_has_tag(node, str8_lit("h_footer"), 0))
      {
        String8List gen_strings = mg_string_list_from_table_gen(mg_arena, table_grid_map, table_col_map, str8_lit(""), node);
        for(String8Node *n = gen_strings.first; n != 0; n = n->next)
        {
          str8_list_push(mg_arena, &layer->h_footer, n->string);
          str8_list_push(mg_arena, &layer->h_footer, str8_lit("\n"));
        }
      }
      if(md_node_has_tag(node, str8_lit("c_header"), 0))
      {
        String8List gen_strings = mg_string_list_from_table_gen(mg_arena, table_grid_map, table_col_map, str8_lit(""), node);
        for(String8Node *n = gen_strings.first; n != 0; n = n->next)
        {
          str8_list_push(mg_arena, &layer->c_header, n->string);
          str8_list_push(mg_arena, &layer->c_header, str8_lit("\n"));
        }
      }
      if(md_node_has_tag(node, str8_lit("c_footer"), 0))
      {
        String8List gen_strings = mg_string_list_from_table_gen(mg_arena, table_grid_map, table_col_map, str8_lit(""), node);
        for(String8Node *n = gen_strings.first; n != 0; n = n->next)
        {
          str8_list_push(mg_arena, &layer->c_footer, n->string);
          str8_list_push(mg_arena, &layer->c_footer, str8_lit("\n"));
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: generate enums
  //
  for(MG_FileParseNode *n = parses.first; n != 0; n = n->next)
  {
    MD_Node *file = n->v.root;
    for(MD_EachNode(node, file->first))
    {
      MD_Node *tag = md_tag_from_string(node, str8_lit("enum"), 0);
      if(!md_node_is_nil(tag))
      {
        String8 enum_name = node->string;
        String8 enum_member_prefix = enum_name;
        if(str8_match(str8_postfix(enum_name, 5), str8_lit("Flags"), 0))
        {
          enum_member_prefix = str8_chop(enum_name, 1);
        }
        String8 enum_base_type_name = tag->first->string;
        String8 layer_key = mg_layer_key_from_path(file->string);
        MG_Layer *layer = mg_layer_from_key(layer_key);
        String8List gen_strings = mg_string_list_from_table_gen(mg_arena, table_grid_map, table_col_map, str8_lit(""), node);
        if(enum_base_type_name.size == 0)
        {
          str8_list_pushf(mg_arena, &layer->enums, "typedef enum %S\n{\n", enum_name);
        }
        else
        {
          str8_list_pushf(mg_arena, &layer->enums, "typedef %S %S;\n", enum_base_type_name, enum_name);
          str8_list_pushf(mg_arena, &layer->enums, "typedef enum %SEnum\n{\n", enum_name);
        }
        for(String8Node *n = gen_strings.first; n != 0; n = n->next)
        {
          String8 escaped = mg_escaped_from_str8(mg_arena, n->string);
          str8_list_pushf(mg_arena, &layer->enums, "%S_%S,\n", enum_member_prefix, escaped);
        }
        if(enum_base_type_name.size == 0)
        {
          str8_list_pushf(mg_arena, &layer->enums, "} %S;\n\n", enum_name);
        }
        else
        {
          str8_list_pushf(mg_arena, &layer->enums, "} %SEnum;\n\n", enum_name);
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: generate xlists
  //
  for(MG_FileParseNode *n = parses.first; n != 0; n = n->next)
  {
    MD_Node *file = n->v.root;
    for(MD_EachNode(node, file->first))
    {
      MD_Node *tag = md_tag_from_string(node, str8_lit("xlist"), 0);
      if(!md_node_is_nil(tag))
      {
        String8 layer_key = mg_layer_key_from_path(file->string);
        MG_Layer *layer = mg_layer_from_key(layer_key);
        String8List gen_strings = mg_string_list_from_table_gen(mg_arena, table_grid_map, table_col_map, str8_lit(""), node);
        str8_list_pushf(mg_arena, &layer->enums, "#define %S \\\n", node->string);
        for(String8Node *n = gen_strings.first; n != 0; n = n->next)
        {
          String8 escaped = mg_escaped_from_str8(mg_arena, n->string);
          str8_list_pushf(mg_arena, &layer->enums, "X(%S)\\\n", escaped);
        }
        str8_list_push(mg_arena, &layer->enums, str8_lit("\n"));
      }
    }
  }
  
  //////////////////////////////
  //- rjf: generate structs
  //
  for(MG_FileParseNode *n = parses.first; n != 0; n = n->next)
  {
    MD_Node *file = n->v.root;
    for(MD_EachNode(node, file->first))
    {
      if(md_node_has_tag(node, str8_lit("struct"), 0))
      {
        String8 layer_key = mg_layer_key_from_path(file->string);
        MG_Layer *layer = mg_layer_from_key(layer_key);
        String8List gen_strings = mg_string_list_from_table_gen(mg_arena, table_grid_map, table_col_map, str8_lit(""), node);
        str8_list_pushf(mg_arena, &layer->structs, "typedef struct %S %S;\n", node->string, node->string);
        str8_list_pushf(mg_arena, &layer->structs, "struct %S\n{\n", node->string);
        for(String8Node *n = gen_strings.first; n != 0; n = n->next)
        {
          String8 escaped = mg_escaped_from_str8(mg_arena, n->string);
          str8_list_pushf(mg_arena, &layer->structs, "%S;\n", escaped);
        }
        str8_list_pushf(mg_arena, &layer->structs, "};\n\n");
      }
    }
  }
  
  //////////////////////////////
  //- rjf: generate data tables
  //
  for(MG_FileParseNode *n = parses.first; n != 0; n = n->next)
  {
    MD_Node *file = n->v.root;
    for(MD_EachNode(node, file->first))
    {
      MD_Node *tag = md_tag_from_string(node, str8_lit("data"), 0);
      if(!md_node_is_nil(tag))
      {
        String8 element_type = tag->first->string;
        String8 layer_key = mg_layer_key_from_path(file->string);
        MG_Layer *layer = mg_layer_from_key(layer_key);
        String8List gen_strings = mg_string_list_from_table_gen(mg_arena, table_grid_map, table_col_map, str8_lit(""), node);
        if(!md_node_has_tag(node, str8_lit("c_file"), 0))
        {
          str8_list_pushf(mg_arena, &layer->h_tables, "extern %S %S[%I64u];\n", element_type, node->string, gen_strings.node_count);
        }
        str8_list_pushf(mg_arena, &layer->c_tables, "%S %S[%I64u] =\n{\n", element_type, node->string, gen_strings.node_count);
        for(String8Node *n = gen_strings.first; n != 0; n = n->next)
        {
          String8 escaped = mg_escaped_from_str8(mg_arena, n->string);
          str8_list_pushf(mg_arena, &layer->c_tables, "%S,\n", escaped);
        }
        str8_list_push(mg_arena, &layer->c_tables, str8_lit("};\n\n"));
      }
    }
  }
  
  //////////////////////////////
  //- rjf: generate enum -> string mapping functions
  //
  for(MG_FileParseNode *n = parses.first; n != 0; n = n->next)
  {
    MD_Node *file = n->v.root;
    for(MD_EachNode(node, file->first))
    {
      MD_Node *tag = md_tag_from_string(node, str8_lit("enum2string_switch"), 0);
      if(!md_node_is_nil(tag))
      {
        String8 enum_type = tag->first->string;
        String8 layer_key = mg_layer_key_from_path(file->string);
        MG_Layer *layer = mg_layer_from_key(layer_key);
        String8List gen_strings = mg_string_list_from_table_gen(mg_arena, table_grid_map, table_col_map, str8_lit(""), node);
        str8_list_pushf(mg_arena, &layer->h_functions, "internal String8 %S(%S v);\n", node->string, enum_type);
        str8_list_pushf(mg_arena, &layer->c_functions, "internal String8\n%S(%S v)\n{\n", node->string, enum_type);
        str8_list_pushf(mg_arena, &layer->c_functions, "String8 result = str8_lit(\"<Unknown %S>\");\n", enum_type);
        str8_list_pushf(mg_arena, &layer->c_functions, "switch(v)\n");
        str8_list_pushf(mg_arena, &layer->c_functions, "{\n");
        str8_list_pushf(mg_arena, &layer->c_functions, "default:{}break;\n");
        for(String8Node *n = gen_strings.first; n != 0; n = n->next)
        {
          String8 escaped = mg_escaped_from_str8(mg_arena, n->string);
          str8_list_pushf(mg_arena, &layer->c_functions, "%S;\n", escaped);
        }
        str8_list_pushf(mg_arena, &layer->c_functions, "}\n");
        str8_list_pushf(mg_arena, &layer->c_functions, "return result;\n");
        str8_list_pushf(mg_arena, &layer->c_functions, "}\n\n");
      }
    }
  }
  
  //////////////////////////////
  //- rjf: generate catch-all generations
  //
  for(MG_FileParseNode *n = parses.first; n != 0; n = n->next)
  {
    MD_Node *file = n->v.root;
    for(MD_EachNode(node, file->first))
    {
      MD_Node *tag = md_tag_from_string(node, str8_lit("gen"), 0);
      if(!md_node_is_nil(tag))
      {
        String8 layer_key = mg_layer_key_from_path(file->string);
        MG_Layer *layer = mg_layer_from_key(layer_key);
        B32 prefer_c_file = md_node_has_tag(node, str8_lit("c_file"), 0);
        String8List *out = prefer_c_file ? &layer->c_catchall : &layer->h_catchall;
        if(tag->first->string.size == 0){}
        else if(str8_match(tag->first->string, str8_lit("enums"), 0))     { out = &layer->enums; }
        else if(str8_match(tag->first->string, str8_lit("structs"), 0))   { out = &layer->structs; }
        else if(str8_match(tag->first->string, str8_lit("functions"), 0)) { out = prefer_c_file ? &layer->c_functions : &layer->h_functions; }
        else if(str8_match(tag->first->string, str8_lit("tables"), 0))    { out = prefer_c_file ? &layer->c_tables : &layer->h_tables; }
        String8List gen_strings = mg_string_list_from_table_gen(mg_arena, table_grid_map, table_col_map, str8_lit(""), node);
        for(String8Node *n = gen_strings.first; n != 0; n = n->next)
        {
          String8 trimmed = str8_skip_chop_whitespace(n->string);
          String8 escaped = mg_escaped_from_str8(mg_arena, trimmed);
          str8_list_push(mg_arena, out, escaped);
          str8_list_push(mg_arena, out, str8_lit("\n"));
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: gather & generate all embeds
  //
  for(MG_FileParseNode *n = parses.first; n != 0; n = n->next)
  {
    MD_Node *file = n->v.root;
    for(MD_EachNode(node, file->first))
    {
      if(md_node_has_tag(node, str8_lit("embed_string"), 0))
      {
        String8 layer_key = mg_layer_key_from_path(file->string);
        MG_Layer *layer = mg_layer_from_key(layer_key);
        String8 embed_string = mg_c_string_literal_from_multiline_string(node->first->string);
        str8_list_pushf(mg_arena, &layer->h_tables, "read_only global String8 %S =\nstr8_lit_comp(\n", node->string);
        str8_list_push (mg_arena, &layer->h_tables, embed_string);
        str8_list_pushf(mg_arena, &layer->h_tables, ");\n\n");
      }
      if(md_node_has_tag(node, str8_lit("embed_file"), 0))
      {
        String8 layer_key = mg_layer_key_from_path(file->string);
        MG_Layer *layer = mg_layer_from_key(layer_key);
        String8 data = os_data_from_file_path(mg_arena, node->first->string);
        String8 embed_string = mg_c_array_literal_contents_from_data(data);
        str8_list_pushf(mg_arena, &layer->h_tables, "read_only global U8 %S__data[] =\n{\n", node->string);
        str8_list_push (mg_arena, &layer->h_tables, embed_string);
        str8_list_pushf(mg_arena, &layer->h_tables, "};\n\n");
        str8_list_pushf(mg_arena, &layer->h_tables, "read_only global String8 %S = {%S__data, sizeof(%S__data)};\n",
                        node->string,
                        node->string,
                        node->string);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: generate all markdown in build folder
  //
  for(MG_FileParseNode *n = parses.first; n != 0; n = n->next)
  {
    MD_Node *file = n->v.root;
    for(MD_EachNode(node, file->first))
    {
      //- rjf: generate markdown page
      if(md_node_has_tag(node, str8_lit("markdown"), 0))
      {
        String8List md_strs = {0};
        for(MD_Node *piece = node->first; !md_node_is_nil(piece); piece = piece->next)
        {
          if(md_node_has_tag(piece, str8_lit("title"), 0))
          {
            str8_list_pushf(mg_arena, &md_strs, "# %S\n\n", piece->string);
          }
          if(md_node_has_tag(piece, str8_lit("subtitle"), 0))
          {
            str8_list_pushf(mg_arena, &md_strs, "## %S\n\n", piece->string);
          }
          if(md_node_has_tag(piece, str8_lit("p"), 0))
          {
            String8 paragraph_text = piece->string;
            String8List paragraph_lines = mg_wrapped_lines_from_string(mg_arena, paragraph_text, 80, 80, 0);
            for(String8Node *n = paragraph_lines.first; n != 0; n = n->next)
            {
              str8_list_push(mg_arena, &md_strs, n->string);
              str8_list_push(mg_arena, &md_strs, str8_lit("\n"));
            }
            str8_list_push(mg_arena, &md_strs, str8_lit("\n"));
          }
          if(md_node_has_tag(piece, str8_lit("unordered_list"), 0))
          {
            String8List gen_strings = mg_string_list_from_table_gen(mg_arena, table_grid_map, table_col_map, str8_lit(""), piece);
            for(String8Node *n = gen_strings.first; n != 0; n = n->next)
            {
              str8_list_pushf(mg_arena, &md_strs, " - ");
              String8 item_text = n->string;
              String8List item_lines = mg_wrapped_lines_from_string(mg_arena, item_text, 80-3, 80, 3);
              for(String8Node *line_n = item_lines.first; line_n != 0; line_n = line_n->next)
              {
                str8_list_push(mg_arena, &md_strs, line_n->string);
                str8_list_pushf(mg_arena, &md_strs, "\n");
              }
            }
            str8_list_pushf(mg_arena, &md_strs, "\n");
          }
        }
        String8 output_path = push_str8f(mg_arena, "%S/%S.md", build_dir_path, node->string);
        FILE *file = fopen((char *)output_path.str, "w");
        for(String8Node *n = md_strs.first; n != 0; n = n->next)
        {
          fwrite(n->string.str, n->string.size, 1, file);
        }
        fclose(file);
      }
    }
  }
  
  //////////////////////////////
  //- rjf: write all layer output files
  //
  DeferLoop(printf("generating layer code..."), printf("\n"))
  {
    for(U64 slot_idx = 0; slot_idx < mg_state->slots_count; slot_idx += 1)
    {
      MG_LayerSlot *slot = &mg_state->slots[slot_idx];
      for(MG_LayerNode *n = slot->first; n != 0; n = n->next)
      {
        MG_Layer *layer = &n->v;
        String8 layer_generated_folder = {0};
        if(layer->gen_folder_name.size != 0)
        {
          String8 gen_folder = layer->gen_folder_name;
          layer_generated_folder = push_str8f(mg_arena, "%S/%S", code_dir_path, gen_folder);
        }
        else
        {
          String8 gen_folder = str8_lit("generated");
          layer_generated_folder = push_str8f(mg_arena, "%S/%S/%S", code_dir_path, layer->key, gen_folder);
        }
        if(os_make_directory(layer_generated_folder))
        {
          String8List layer_key_parts = str8_split_path(mg_arena, layer->key);
          StringJoin join = {0};
          join.sep = str8_lit("_");
          String8 layer_key_filename = str8_list_join(mg_arena, &layer_key_parts, &join);
          String8 layer_key_filename_upper = upper_from_str8(mg_arena, layer_key_filename);
          String8 h_path = push_str8f(mg_arena, "%S/%S.meta.h", layer_generated_folder, layer_key_filename);
          String8 c_path = push_str8f(mg_arena, "%S/%S.meta.c", layer_generated_folder, layer_key_filename);
          if(layer->h_name_override.size != 0)
          {
            h_path = push_str8f(mg_arena, "%S/%S", layer_generated_folder, str8_skip_last_slash(layer->h_name_override));
          }
          if(layer->c_name_override.size != 0)
          {
            c_path = push_str8f(mg_arena, "%S/%S", layer_generated_folder, str8_skip_last_slash(layer->c_name_override));
          }
          {
            FILE *h = fopen((char *)h_path.str, "w");
            fprintf(h, "// Copyright (c) 2024 Epic Games Tools\n");
            fprintf(h, "// Licensed under the MIT license (https://opensource.org/license/mit/)\n\n");
            if(layer->h_header.first == 0)
            {
              fprintf(h, "//- GENERATED CODE\n\n");
              fprintf(h, "#ifndef %.*s_META_H\n", str8_varg(layer_key_filename_upper));
              fprintf(h, "#define %.*s_META_H\n\n", str8_varg(layer_key_filename_upper));
            }
            else for(String8Node *n = layer->h_header.first; n != 0; n = n->next)
            {
              fwrite(n->string.str, n->string.size, 1, h);
            }
            for(String8Node *n = layer->enums.first; n != 0; n = n->next)
            {
              fwrite(n->string.str, n->string.size, 1, h);
            }
            for(String8Node *n = layer->structs.first; n != 0; n = n->next)
            {
              fwrite(n->string.str, n->string.size, 1, h);
            }
            for(String8Node *n = layer->h_catchall.first; n != 0; n = n->next)
            {
              fwrite(n->string.str, n->string.size, 1, h);
            }
            for(String8Node *n = layer->h_functions.first; n != 0; n = n->next)
            {
              fwrite(n->string.str, n->string.size, 1, h);
            }
            if(layer->h_tables.first != 0)
            {
              if(!layer->is_library)
              {
                fprintf(h, "C_LINKAGE_BEGIN\n");
              }
              for(String8Node *n = layer->h_tables.first; n != 0; n = n->next)
              {
                fwrite(n->string.str, n->string.size, 1, h);
              }
              fprintf(h, "\n");
              if(!layer->is_library)
              {
                fprintf(h, "C_LINKAGE_END\n\n");
              }
            }
            if(layer->h_footer.first == 0)
            {
              fprintf(h, "#endif // %.*s_META_H\n", str8_varg(layer_key_filename_upper));
            }
            else for(String8Node *n = layer->h_footer.first; n != 0; n = n->next)
            {
              fwrite(n->string.str, n->string.size, 1, h);
            }
            fclose(h);
          }
          {
            FILE *c = fopen((char *)c_path.str, "w");
            fprintf(c, "// Copyright (c) 2024 Epic Games Tools\n");
            fprintf(c, "// Licensed under the MIT license (https://opensource.org/license/mit/)\n\n");
            if(layer->c_header.first == 0)
            {
              fprintf(c, "//- GENERATED CODE\n\n");
            }
            else for(String8Node *n = layer->c_header.first; n != 0; n = n->next)
            {
              fwrite(n->string.str, n->string.size, 1, c);
            }
            for(String8Node *n = layer->c_catchall.first; n != 0; n = n->next)
            {
              fwrite(n->string.str, n->string.size, 1, c);
            }
            if(layer->c_tables.first != 0)
            {
              if(!layer->is_library)
              {
                fprintf(c, "C_LINKAGE_BEGIN\n");
              }
              for(String8Node *n = layer->c_tables.first; n != 0; n = n->next)
              {
                fwrite(n->string.str, n->string.size, 1, c);
              }
              if(!layer->is_library)
              {
                fprintf(c, "C_LINKAGE_END\n\n");
              }
            }
            for(String8Node *n = layer->c_functions.first; n != 0; n = n->next)
            {
              fwrite(n->string.str, n->string.size, 1, c);
            }
            if(layer->c_footer.first != 0)
            {
              for(String8Node *n = layer->c_footer.first; n != 0; n = n->next)
              {
                fwrite(n->string.str, n->string.size, 1, c);
              }
            }
            fclose(c);
          }
        }
      }
    }
  }
  
  //////////////////////////////
  //- rjf: write out all messages to stderr
  //
  for(MG_MsgNode *n = msgs.first; n != 0; n = n->next)
  {
    MG_Msg *msg = &n->v;
    fprintf(stderr, "%.*s: %.*s: %.*s\n", str8_varg(msg->location), str8_varg(msg->kind), str8_varg(msg->msg));
  }
}
