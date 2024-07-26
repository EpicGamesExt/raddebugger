// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: String Expression Operator Tables

read_only global String8 mg_str_expr_op_symbol_string_table[MG_StrExprOp_COUNT] =
{
  str8_lit_comp(""),
  str8_lit_comp("."),  // MG_StrExprOp_Dot
  str8_lit_comp("->"), // MG_StrExprOp_ExpandIfTrue
  str8_lit_comp(".."), // MG_StrExprOp_Concat
  str8_lit_comp("=>"), // MG_StrExprOp_BumpToColumn
  str8_lit_comp("+"),  // MG_StrExprOp_Add
  str8_lit_comp("-"),  // MG_StrExprOp_Subtract
  str8_lit_comp("*"),  // MG_StrExprOp_Multiply
  str8_lit_comp("/"),  // MG_StrExprOp_Divide
  str8_lit_comp("%"),  // MG_StrExprOp_Modulo
  str8_lit_comp("<<"), // MG_StrExprOp_LeftShift
  str8_lit_comp(">>"), // MG_StrExprOp_RightShift
  str8_lit_comp("&"),  // MG_StrExprOp_BitwiseAnd
  str8_lit_comp("|"),  // MG_StrExprOp_BitwiseOr
  str8_lit_comp("^"),  // MG_StrExprOp_BitwiseXor
  str8_lit_comp("~"),  // MG_StrExprOp_BitwiseNegate
  str8_lit_comp("&&"), // MG_StrExprOp_BooleanAnd
  str8_lit_comp("||"), // MG_StrExprOp_BooleanOr
  str8_lit_comp("!"),  // MG_StrExprOp_BooleanNot
  str8_lit_comp("=="), // MG_StrExprOp_Equals
  str8_lit_comp("!="), // MG_StrExprOp_DoesNotEqual
};

read_only global S8 mg_str_expr_op_precedence_table[MG_StrExprOp_COUNT] =
{
  0,
  20, // MG_StrExprOp_Dot
  1,  // MG_StrExprOp_ExpandIfTrue
  2,  // MG_StrExprOp_Concat
  12, // MG_StrExprOp_BumpToColumn
  5,  // MG_StrExprOp_Add
  5,  // MG_StrExprOp_Subtract
  6,  // MG_StrExprOp_Multiply
  6,  // MG_StrExprOp_Divide
  6,  // MG_StrExprOp_Modulo
  7,  // MG_StrExprOp_LeftShift
  7,  // MG_StrExprOp_RightShift
  8,  // MG_StrExprOp_BitwiseAnd
  10, // MG_StrExprOp_BitwiseOr
  9,  // MG_StrExprOp_BitwiseXor
  11, // MG_StrExprOp_BitwiseNegate
  3,  // MG_StrExprOp_BooleanAnd
  3,  // MG_StrExprOp_BooleanOr
  11, // MG_StrExprOp_BooleanNot
  4,  // MG_StrExprOp_Equals
  4,  // MG_StrExprOp_DoesNotEqual
};

read_only global MG_StrExprOpKind mg_str_expr_op_kind_table[MG_StrExprOp_COUNT] =
{
  MG_StrExprOpKind_Null,
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Dot
  MG_StrExprOpKind_Binary, // MG_StrExprOp_ExpandIfTrue
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Concat
  MG_StrExprOpKind_Prefix, // MG_StrExprOp_BumpToColumn
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Add
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Subtract
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Multiply
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Divide
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Modulo
  MG_StrExprOpKind_Binary, // MG_StrExprOp_LeftShift
  MG_StrExprOpKind_Binary, // MG_StrExprOp_RightShift
  MG_StrExprOpKind_Binary, // MG_StrExprOp_BitwiseAnd
  MG_StrExprOpKind_Binary, // MG_StrExprOp_BitwiseOr
  MG_StrExprOpKind_Binary, // MG_StrExprOp_BitwiseXor
  MG_StrExprOpKind_Prefix, // MG_StrExprOp_BitwiseNegate
  MG_StrExprOpKind_Binary, // MG_StrExprOp_BooleanAnd
  MG_StrExprOpKind_Binary, // MG_StrExprOp_BooleanOr
  MG_StrExprOpKind_Prefix, // MG_StrExprOp_BooleanNot
  MG_StrExprOpKind_Binary, // MG_StrExprOp_Equals
  MG_StrExprOpKind_Binary, // MG_StrExprOp_DoesNotEqual
};

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
mg_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

internal TxtPt
mg_txt_pt_from_string_off(String8 string, U64 off)
{
  TxtPt pt = txt_pt(1, 1);
  for(U64 idx = 0; idx < string.size && idx < off; idx += 1)
  {
    if(string.str[idx] == '\n')
    {
      pt.line += 1;
      pt.column = 1;
    }
    else
    {
      pt.column += 1;
    }
  }
  return pt;
}

////////////////////////////////
//~ rjf: Message Lists

internal void
mg_msg_list_push(Arena *arena, MG_MsgList *msgs, MG_Msg *msg)
{
  MG_MsgNode *n = push_array(arena, MG_MsgNode, 1);
  MemoryCopyStruct(&n->v, msg);
  SLLQueuePush(msgs->first, msgs->last, n);
  msgs->count += 1;
}

////////////////////////////////
//~ rjf: String Escaping

internal String8
mg_escaped_from_str8(Arena *arena, String8 string)
{
  // NOTE(rjf): This doesn't handle hex/octal/unicode escape sequences right
  // now, just the simple stuff.
  Temp scratch = scratch_begin(&arena, 1);
  String8List strs = {0};
  U64 start = 0;
  for(U64 idx = 0; idx <= string.size; idx += 1)
  {
    if(idx == string.size || string.str[idx] == '\\' || string.str[idx] == '\r')
    {
      String8 str = str8_substr(string, r1u64(start, idx));
      if(str.size != 0)
      {
        str8_list_push(arena, &strs, str);
      }
      start = idx+1;
    }
    if(idx < string.size && string.str[idx] == '\\')
    {
      U8 next_char = string.str[idx+1];
      U8 replace_byte = 0;
      switch(next_char)
      {
        default:{}break;
        case 'a': replace_byte = 0x07; break;
        case 'b': replace_byte = 0x08; break;
        case 'e': replace_byte = 0x1b; break;
        case 'f': replace_byte = 0x0c; break;
        case 'n': replace_byte = 0x0a; break;
        case 'r': replace_byte = 0x0d; break;
        case 't': replace_byte = 0x09; break;
        case 'v': replace_byte = 0x0b; break;
        case '\\':replace_byte = '\\'; break;
        case '\'':replace_byte = '\''; break;
        case '"': replace_byte = '"';  break;
        case '?': replace_byte = '?';  break;
      }
      String8 replace_string = push_str8_copy(scratch.arena, str8(&replace_byte, 1));
      str8_list_push(scratch.arena, &strs, replace_string);
      if(replace_byte == '\\' || replace_byte == '"' || replace_byte == '\'')
      {
        idx += 1;
        start += 1;
      }
    }
  }
  String8 result = str8_list_join(arena, &strs, 0);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: String Wrapping

internal String8List
mg_wrapped_lines_from_string(Arena *arena, String8 string, U64 first_line_max_width, U64 max_width, U64 wrap_indent)
{
  String8List list = {0};
  Rng1U64 line_range = r1u64(0, 0);
  U64 wrapped_indent_level = 0;
  static char *spaces = "                                                                ";
  for (U64 idx = 0; idx <= string.size; idx += 1){
    U8 chr = idx < string.size ? string.str[idx] : 0;
    if (chr == '\n'){
      Rng1U64 candidate_line_range = line_range;
      candidate_line_range.max = idx;
      // NOTE(nick): when wrapping is interrupted with \n we emit a string without including \n
      // because later tool_fprint_list inserts separator after each node
      // except for last node, so don't strip last \n.
      if (idx + 1 == string.size){
        candidate_line_range.max += 1;
      }
      String8 substr = str8_substr(string, candidate_line_range);
      str8_list_push(arena, &list, substr);
      line_range = r1u64(idx+1,idx+1);
    }
    else
      if (char_is_space(chr) || chr == 0){
      Rng1U64 candidate_line_range = line_range;
      candidate_line_range.max = idx;
      String8 substr = str8_substr(string, candidate_line_range);
      U64 width_this_line = max_width-wrapped_indent_level;
      if (list.node_count == 0){
        width_this_line = first_line_max_width;
      }
      if (substr.size > width_this_line){
        String8 line = str8_substr(string, line_range);
        if (wrapped_indent_level > 0){
          line = push_str8f(arena, "%.*s%S", wrapped_indent_level, spaces, line);
        }
        str8_list_push(arena, &list, line);
        line_range = r1u64(line_range.max+1, candidate_line_range.max);
        wrapped_indent_level = ClampTop(64, wrap_indent);
      }
      else{
        line_range = candidate_line_range;
      }
    }
  }
  if (line_range.min < string.size && line_range.max > line_range.min){
    String8 line = str8_substr(string, line_range);
    if (wrapped_indent_level > 0){
      line = push_str8f(arena, "%.*s%S", wrapped_indent_level, spaces, line);
    }
    str8_list_push(arena, &list, line);
  }
  return list;
}

////////////////////////////////
//~ rjf: C-String-Izing

internal String8
mg_c_string_literal_from_multiline_string(String8 string)
{
  String8List strings = {0};
  {
    str8_list_push(mg_arena, &strings, str8_lit("\"\"\n"));
    U64 active_line_start_off = 0;
    for(U64 off = 0; off <= string.size; off += 1)
    {
      B32 is_newline = (off < string.size && (string.str[off] == '\n' || string.str[off] == '\r'));
      B32 is_ender = (off >= string.size || is_newline);
      if(is_ender)
      {
        String8 line = str8_substr(string, r1u64(active_line_start_off, off));
        str8_list_push(mg_arena, &strings, str8_lit("\""));
        str8_list_push(mg_arena, &strings, line);
        if(is_newline)
        {
          str8_list_push(mg_arena, &strings, str8_lit("\\n\"\n"));
        }
        else
        {
          str8_list_push(mg_arena, &strings, str8_lit("\"\n"));
        }
        active_line_start_off = off+1;
      }
      if(is_newline && string.str[off] == '\r')
      {
        active_line_start_off += 1;
        off += 1;
      }
    }
  }
  String8 result = str8_list_join(mg_arena, &strings, 0);
  return result;
}

internal String8
mg_c_array_literal_contents_from_data(String8 data)
{
  Temp scratch = scratch_begin(0, 0);
  String8List strings = {0};
  {
    for(U64 off = 0; off < data.size;)
    {
      U64 chunk_size = Min(data.size-off, 64);
      U8 *chunk_bytes = data.str+off;
      String8 chunk_text_string = {0};
      chunk_text_string.size = chunk_size*5;
      chunk_text_string.str = push_array(mg_arena, U8, chunk_text_string.size);
      for(U64 byte_idx = 0; byte_idx < chunk_size; byte_idx += 1)
      {
        String8 byte_str = push_str8f(scratch.arena, "0x%02x,", chunk_bytes[byte_idx]);
        MemoryCopy(chunk_text_string.str+byte_idx*5, byte_str.str, byte_str.size);
      }
      off += chunk_size;
      str8_list_push(mg_arena, &strings, chunk_text_string);
      str8_list_push(mg_arena, &strings, str8_lit("\n"));
    }
  }
  String8 result = str8_list_join(mg_arena, &strings, 0);
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Map Functions

internal MG_Map
mg_push_map(Arena *arena, U64 slot_count)
{
  MG_Map map = {0};
  map.slots_count = slot_count;
  map.slots = push_array(arena, MG_MapSlot, map.slots_count);
  return map;
}

internal void *
mg_map_ptr_from_string(MG_Map *map, String8 string)
{
  void *result = 0;
  {
    U64 hash = mg_hash_from_string(string);
    U64 slot_idx = hash%map->slots_count;
    MG_MapSlot *slot = &map->slots[slot_idx];
    for(MG_MapNode *n = slot->first; n != 0; n = n->next)
    {
      if(str8_match(n->key, string, 0))
      {
        result = n->val;
        break;
      }
    }
  }
  return result;
}

internal void
mg_map_insert_ptr(Arena *arena, MG_Map *map, String8 string, void *val)
{
  U64 hash = mg_hash_from_string(string);
  U64 slot_idx = hash%map->slots_count;
  MG_MapSlot *slot = &map->slots[slot_idx];
  MG_MapNode *n = push_array(arena, MG_MapNode, 1);
  n->key = push_str8_copy(arena, string);
  n->val = val;
  SLLQueuePush(slot->first, slot->last, n);
}

////////////////////////////////
//~ rjf: String Expression Parsing

internal MG_StrExpr *
mg_push_str_expr(Arena *arena, MG_StrExprOp op, MD_Node *node)
{
  MG_StrExpr *expr = push_array(arena, MG_StrExpr, 1);
  MemoryCopyStruct(expr, &mg_str_expr_nil);
  expr->op = op;
  expr->node = node;
  return expr;
}

internal MG_StrExprParseResult
mg_str_expr_parse_from_first_opl__min_prec(Arena *arena, MD_Node *first, MD_Node *opl, S8 min_prec)
{
  MG_StrExprParseResult parse = {&mg_str_expr_nil};
  {
    MD_Node *it = first;
    
    //- rjf: consume prefix operators
    MG_StrExpr *leafmost_op = &mg_str_expr_nil;
    for(;it != opl && !md_node_is_nil(it);)
    {
      MG_StrExprOp found_op = MG_StrExprOp_Null;
      for(MG_StrExprOp op = (MG_StrExprOp)(MG_StrExprOp_Null+1);
          op < MG_StrExprOp_COUNT;
          op = (MG_StrExprOp)(op+1))
      {
        if(mg_str_expr_op_kind_table[op] == MG_StrExprOpKind_Prefix &&
           str8_match(it->string, mg_str_expr_op_symbol_string_table[op], 0) &&
           mg_str_expr_op_precedence_table[op] >= min_prec)
        {
          found_op = op;
          break;
        }
      }
      if(found_op != MG_StrExprOp_Null)
      {
        MG_StrExpr *op_expr = mg_push_str_expr(arena, found_op, it);
        if(leafmost_op == &mg_str_expr_nil)
        {
          leafmost_op = op_expr;
        }
        op_expr->left = parse.root;
        parse.root = op_expr;
        it = it->next;
      }
      else
      {
        break;
      }
    }
    
    //- rjf: parse atom
    {
      MG_StrExpr *atom = &mg_str_expr_nil;
      if(it->flags & (MD_NodeFlag_Identifier|MD_NodeFlag_Numeric|MD_NodeFlag_StringLiteral) &&
         md_node_is_nil(it->first))
      {
        atom = mg_push_str_expr(arena, MG_StrExprOp_Null, it);
        it = it->next;
      }
      else if(!md_node_is_nil(it->first))
      {
        MG_StrExprParseResult subparse = mg_str_expr_parse_from_first_opl__min_prec(arena, it->first, &md_nil_node, 0);
        atom = subparse.root;
        md_msg_list_concat_in_place(&parse.msgs, &subparse.msgs);
        it = it->next;
      }
      if(leafmost_op != &mg_str_expr_nil)
      {
        leafmost_op->left = atom;
      }
      else
      {
        parse.root = atom;
      }
    }
    
    //- rjf: parse binary operator extensions at this precedence level
    for(;it != opl && !md_node_is_nil(it);)
    {
      // rjf: find binary op kind of `it`
      MG_StrExprOp found_op = MG_StrExprOp_Null;
      for(MG_StrExprOp op = (MG_StrExprOp)(MG_StrExprOp_Null+1);
          op < MG_StrExprOp_COUNT;
          op = (MG_StrExprOp)(op+1))
      {
        if(mg_str_expr_op_kind_table[op] == MG_StrExprOpKind_Binary &&
           str8_match(it->string, mg_str_expr_op_symbol_string_table[op], 0) &&
           mg_str_expr_op_precedence_table[op] >= min_prec)
        {
          found_op = op;
          break;
        }
      }
      
      // rjf: good found_op -> build binary expr
      if(found_op != MG_StrExprOp_Null)
      {
        MG_StrExpr *op_expr = mg_push_str_expr(arena, found_op, it);
        if(leafmost_op == &mg_str_expr_nil)
        {
          leafmost_op = op_expr;
        }
        op_expr->left = parse.root;
        parse.root = op_expr;
        it = it->next;
      }
      else
      {
        break;
      }
      
      // rjf: parse right hand side of binary operator
      MG_StrExprParseResult subparse = mg_str_expr_parse_from_first_opl__min_prec(arena, it, opl, mg_str_expr_op_precedence_table[found_op]+1);
      parse.root->right = subparse.root;
      md_msg_list_concat_in_place(&parse.msgs, &subparse.msgs);
      if(subparse.root == &mg_str_expr_nil)
      {
        md_msg_list_pushf(arena, &parse.msgs, it, MD_MsgKind_Error, "Missing right-hand-side of '%S'.", mg_str_expr_op_symbol_string_table[found_op]);
      }
      it = subparse.next_node;
    }
    
    // rjf: store next node for more caller-side parsing
    parse.next_node = it;
  }
  return parse;
}

internal MG_StrExprParseResult
mg_str_expr_parse_from_first_opl(Arena *arena, MD_Node *first, MD_Node *opl)
{
  MG_StrExprParseResult parse = mg_str_expr_parse_from_first_opl__min_prec(arena, first, opl, 0);
  return parse;
}

internal MG_StrExprParseResult
mg_str_expr_parse_from_root(Arena *arena, MD_Node *root)
{
  MG_StrExprParseResult parse = mg_str_expr_parse_from_first_opl__min_prec(arena, root->first, &md_nil_node, 0);
  return parse;
}

////////////////////////////////
//~ rjf: Table Generation Functions

internal MG_NodeArray
mg_node_array_make(Arena *arena, U64 count)
{
  MG_NodeArray result = {0};
  result.count = count;
  result.v = push_array(arena, MD_Node *, result.count);
  for(U64 idx = 0; idx < result.count; idx += 1)
  {
    result.v[idx] = &md_nil_node;
  }
  return result;
}

internal MG_NodeArray
mg_child_array_from_node(Arena *arena, MD_Node *node)
{
  MG_NodeArray children = mg_node_array_make(arena, md_child_count_from_node(node));
  U64 idx = 0;
  for(MD_EachNode(child, node->first))
  {
    children.v[idx] = child;
    idx += 1;
  }
  return children;
}

internal MG_NodeGrid
mg_node_grid_make_from_node(Arena *arena, MD_Node *root)
{
  MG_NodeGrid grid = {0};
  
  // rjf: determine dimensions
  U64 row_count = md_child_count_from_node(root);
  U64 column_count = 0;
  for(MD_EachNode(row, root->first))
  {
    U64 cell_count_this_row = md_child_count_from_node(row);
    column_count = Max(column_count, cell_count_this_row);
  }
  
  // rjf: fill grid
  grid.x_stride = 1;
  grid.y_stride = column_count;
  grid.cells = mg_node_array_make(arena, row_count*column_count);
  grid.row_parents = mg_node_array_make(arena, row_count);
  
  // rjf: fill nodes
  {
    U64 y = 0;
    for(MD_EachNode(row, root->first))
    {
      U64 x = 0;
      grid.row_parents.v[y] = row;
      for(MD_EachNode(cell, row->first))
      {
        grid.cells.v[x*grid.x_stride + y*grid.y_stride] = cell;
        x += 1;
      }
      y += 1;
    }
  }
  
  return grid;
}

internal MG_NodeArray
mg_row_from_index(MG_NodeGrid grid, U64 index)
{
  MG_NodeArray result = {0};
  if(0 <= index && index < grid.cells.count / grid.x_stride)
  {
    result.count = grid.y_stride;
    result.v = &grid.cells.v[index*grid.y_stride];
  }
  return result;
}

internal MG_NodeArray
mg_column_from_index(Arena *arena, MG_NodeGrid grid, U64 index)
{
  MG_NodeArray result = {0};
  if(0 <= index && index < grid.y_stride)
  {
    U64 row_count = grid.cells.count / grid.y_stride;
    result = mg_node_array_make(arena, row_count);
    U64 idx = 0;
    for(U64 row_idx = 0; row_idx < row_count; row_idx += 1, idx += 1)
    {
      result.v[idx] = grid.cells.v[index*grid.x_stride + row_idx*grid.y_stride];
    }
  }
  return result;
}

internal MD_Node *
mg_node_from_grid_xy(MG_NodeGrid grid, U64 x, U64 y)
{
  MD_Node *result = &md_nil_node;
  U64 idx = x*grid.x_stride + y*grid.y_stride;
  if(0 <= idx && idx < grid.cells.count)
  {
    result = grid.cells.v[idx];
  }
  return result;
}

internal MG_ColumnDescArray
mg_column_desc_array_make(Arena *arena, U64 count, MG_ColumnDesc *descs)
{
  MG_ColumnDescArray result = {0};
  result.count = count;
  result.v = push_array(arena, MG_ColumnDesc, result.count);
  MemoryCopy(result.v, descs, sizeof(*result.v)*result.count);
  return result;
}

internal MG_ColumnDescArray
mg_column_desc_array_from_tag(Arena *arena, MD_Node *tag)
{
  MG_ColumnDescArray result = {0};
  result.count = md_child_count_from_node(tag);
  result.v = push_array(arena, MG_ColumnDesc, result.count);
  U64 idx = 0;
  for(MD_EachNode(hdr, tag->first))
  {
    result.v[idx].name = push_str8_copy(arena, hdr->string);
    result.v[idx].kind = MG_ColumnKind_DirectCell;
    if(md_node_has_tag(hdr, str8_lit("tag_check"), 0))
    {
      result.v[idx].kind = MG_ColumnKind_CheckForTag;
    }
    if(md_node_has_tag(hdr, str8_lit("tag_child"), 0))
    {
      String8 tag_name = md_tag_from_string(hdr, str8_lit("tag_child"), 0)->first->string;
      result.v[idx].kind = MG_ColumnKind_TagChild;
      result.v[idx].tag_name = tag_name;
    }
    idx += 1;
  }
  return result;
}

internal U64
mg_column_index_from_name(MG_ColumnDescArray descs, String8 name)
{
  U64 result = 0;
  for(U64 idx = 0; idx < descs.count; idx += 1)
  {
    if(str8_match(descs.v[idx].name, name, 0))
    {
      result = idx;
      break;
    }
  }
  return result;
}

internal String8
mg_string_from_row_desc_idx(MD_Node *row_parent, MG_ColumnDescArray descs, U64 idx)
{
  String8 result = {0};
  
  // rjf: grab relevant column description
  MG_ColumnDesc *desc = 0;
  if(0 <= idx && idx < descs.count)
  {
    desc = descs.v + idx;
  }
  
  // rjf: grab node
  if(desc != 0)
  {
    switch(desc->kind)
    {
      default: break;
      
      case MG_ColumnKind_DirectCell:
      {
        // rjf: determine grid idx (shifted by synthetic columns)
        U64 cell_idx = idx;
        for(U64 col_idx = 0; col_idx < descs.count && col_idx < idx; col_idx += 1)
        {
          if(descs.v[col_idx].kind != MG_ColumnKind_DirectCell)
          {
            cell_idx -= 1;
          }
        }
        MD_Node *node = md_child_from_index(row_parent, cell_idx);
        result = node->string;
      }break;
      
      case MG_ColumnKind_CheckForTag:
      {
        String8 tag_name = desc->name;
        MD_Node *tag = md_tag_from_string(row_parent, tag_name, 0);
        result = md_node_is_nil(tag) ? str8_lit("0") : str8_lit("1");
      }break;
      
      case MG_ColumnKind_TagChild:
      {
        String8 tag_name = desc->tag_name;
        MD_Node *tag = md_tag_from_string(row_parent, tag_name, 0);
        result = tag->first->string;
      }break;
    }
  }
  
  return result;
}

internal S64
mg_eval_table_expand_expr__numeric(MG_StrExpr *expr, MG_TableExpandInfo *info)
{
  S64 result = 0;
  MG_StrExprOp op = expr->op;
  
  switch(op)
  {
    default:
    {
      if(MG_StrExprOp_FirstString <= op && op <= MG_StrExprOp_LastString)
      {
        Temp scratch = scratch_begin(0, 0);
        String8List result_strs = {0};
        mg_eval_table_expand_expr__string(scratch.arena, expr, info, &result_strs);
        String8 result_str = str8_list_join(scratch.arena, &result_strs, 0);
        try_s64_from_str8_c_rules(result_str, &result);
        scratch_end(scratch);
      }
    }break;
    
    case MG_StrExprOp_Null:
    {
      try_s64_from_str8_c_rules(expr->node->string, &result);
    }break;
    
    //- rjf: numeric arithmetic binary ops
    case MG_StrExprOp_Add:
    case MG_StrExprOp_Subtract:
    case MG_StrExprOp_Multiply:
    case MG_StrExprOp_Divide:
    case MG_StrExprOp_Modulo:
    case MG_StrExprOp_LeftShift:
    case MG_StrExprOp_RightShift:
    case MG_StrExprOp_BitwiseAnd:
    case MG_StrExprOp_BitwiseOr:
    case MG_StrExprOp_BitwiseXor:
    case MG_StrExprOp_BooleanAnd:
    case MG_StrExprOp_BooleanOr:
    {
      S64 left_val = mg_eval_table_expand_expr__numeric(expr->left, info);
      S64 right_val = mg_eval_table_expand_expr__numeric(expr->right, info);
      switch(op)
      {
        default:break;
        case MG_StrExprOp_Add:        result = left_val+right_val;  break;
        case MG_StrExprOp_Subtract:   result = left_val-right_val;  break;
        case MG_StrExprOp_Multiply:   result = left_val*right_val;  break;
        case MG_StrExprOp_Divide:     result = left_val/right_val;  break;
        case MG_StrExprOp_Modulo:     result = left_val%right_val;  break;
        case MG_StrExprOp_LeftShift:  result = left_val<<right_val; break;
        case MG_StrExprOp_RightShift: result = left_val>>right_val; break;
        case MG_StrExprOp_BitwiseAnd: result = left_val&right_val;  break;
        case MG_StrExprOp_BitwiseOr:  result = left_val|right_val;  break;
        case MG_StrExprOp_BitwiseXor: result = left_val^right_val;  break;
        case MG_StrExprOp_BooleanAnd: result = left_val&&right_val; break;
        case MG_StrExprOp_BooleanOr:  result = left_val||right_val; break;
      }
    }break;
    
    //- rjf: prefix unary ops
    case MG_StrExprOp_BitwiseNegate:
    case MG_StrExprOp_BooleanNot:
    {
      S64 right_val = mg_eval_table_expand_expr__numeric(expr->left, info);
      switch(op)
      {
        default:break;
        case MG_StrExprOp_BitwiseNegate: result = (S64)(~((U64)right_val)); break;
        case MG_StrExprOp_BooleanNot:    result = !right_val;
      }
    }break;
    
    //- rjf: comparisons
    case MG_StrExprOp_Equals:
    case MG_StrExprOp_DoesNotEqual:
    {
      Temp scratch = scratch_begin(0, 0);
      String8List left_strs = {0};
      String8List right_strs = {0};
      mg_eval_table_expand_expr__string(scratch.arena, expr->left, info, &left_strs);
      mg_eval_table_expand_expr__string(scratch.arena, expr->right, info, &right_strs);
      String8 left_str = str8_list_join(scratch.arena, &left_strs, 0);
      String8 right_str = str8_list_join(scratch.arena, &right_strs, 0);
      B32 match = str8_match(left_str, right_str, 0);
      result = (op == MG_StrExprOp_Equals ? match : !match);
      scratch_end(scratch);
    }break;
  }
  
  return result;
}

internal void
mg_eval_table_expand_expr__string(Arena *arena, MG_StrExpr *expr, MG_TableExpandInfo *info, String8List *out)
{
  MG_StrExprOp op = expr->op;
  
  switch(op)
  {
    default:
    {
      if(MG_StrExprOp_FirstNumeric <= op && op <= MG_StrExprOp_LastNumeric)
      {
        S64 numeric_eval = mg_eval_table_expand_expr__numeric(expr, info);
        String8 numeric_eval_stringized = {0};
        if(md_node_has_tag(md_root_from_node(expr->node), str8_lit("hex"), 0))
        {
          numeric_eval_stringized = push_str8f(arena, "0x%I64x", numeric_eval);
        }
        else
        {
          numeric_eval_stringized = push_str8f(arena, "%I64d", numeric_eval);
        }
        str8_list_push(arena, out, numeric_eval_stringized);
      }
    }break;
    
    case MG_StrExprOp_Null:
    {
      str8_list_push(arena, out, expr->node->string);
    }break;
    
    case MG_StrExprOp_Dot:
    {
      // rjf: grab left/right
      MG_StrExpr *left_expr = expr->left;
      MD_Node *left_node = left_expr->node;
      MG_StrExpr *right_expr = expr->right;
      MD_Node *right_node = right_expr->node;
      
      // rjf: grab table name (LHS of .) and column lookup string (RHS of .)
      String8 expand_label = left_node->string;
      String8 column_lookup = right_node->string;
      
      // rjf: find which task corresponds to this table
      U64 row_idx = 0;
      MG_NodeGrid *grid = 0;
      MG_ColumnDescArray column_descs = {0};
      {
        for(MG_TableExpandTask *task = info->first_expand_task; task != 0; task = task->next)
        {
          if(str8_match(expand_label, task->expansion_label, 0))
          {
            row_idx = task->idx;
            grid = task->grid;
            column_descs = task->column_descs;
            break;
          }
        }
      }
      
      // rjf: grab row parent
      MD_Node *row_parent = &md_nil_node;
      if(grid && (0 <= row_idx && row_idx < grid->row_parents.count))
      {
        row_parent = grid->row_parents.v[row_idx];
      }
      
      // rjf: get string for this table lookup
      String8 lookup_string = {0};
      {
        U64 column_idx = 0;
        
        if(str8_match(column_lookup, str8_lit("_it"), 0))
        {
          lookup_string = push_str8f(arena, "%I64u", row_idx);
        }
        else
        {
          // NOTE(rjf): numeric column lookup (column index)
          if(right_node->flags & MD_NodeFlag_Numeric)
          {
            try_u64_from_str8_c_rules(column_lookup, &column_idx);
          }
          
          // NOTE(rjf): string column lookup (column name)
          if(right_node->flags & (MD_NodeFlag_Identifier|MD_NodeFlag_StringLiteral))
          {
            column_idx = mg_column_index_from_name(column_descs, column_lookup);
          }
          
          lookup_string = mg_string_from_row_desc_idx(row_parent, column_descs, column_idx);
          if(str8_match(lookup_string, str8_lit("--"), 0))
          {
            lookup_string = info->missing_value_fallback;
          }
        }
      }
      
      // rjf: push lookup string
      {
        str8_list_push(arena, out, lookup_string);
      }
    }break;
    
    case MG_StrExprOp_ExpandIfTrue:
    {
      S64 bool_value = mg_eval_table_expand_expr__numeric(expr->left, info);
      if(bool_value)
      {
        mg_eval_table_expand_expr__string(arena, expr->right, info, out);
      }
    }break;
    
    case MG_StrExprOp_Concat:
    {
      mg_eval_table_expand_expr__string(arena, expr->left, info, out);
      mg_eval_table_expand_expr__string(arena, expr->right, info, out);
    }break;
    
    case MG_StrExprOp_BumpToColumn:
    {
      S64 column = mg_eval_table_expand_expr__numeric(expr->left, info);
      S64 current_column = out->total_size;
      S64 spaces_to_push = column - current_column;
      if(spaces_to_push > 0)
      {
        String8 str = {0};
        str.size = spaces_to_push;
        str.str = push_array(arena, U8, spaces_to_push);
        for(S64 idx = 0; idx < spaces_to_push; idx += 1)
        {
          str.str[idx] = ' ';
        }
        str8_list_push(arena, out, str);
      }
    }break;
  }
}

internal void
mg_loop_table_column_expansion(Arena *arena, String8 strexpr, MG_TableExpandInfo *info, MG_TableExpandTask *task, String8List *out)
{
  Temp scratch = scratch_begin(&arena, 1);
  for(U64 it_idx = 0; it_idx < task->count; it_idx += 1)
  {
    task->idx = it_idx;
    
    //- rjf: iterate all further dimensions, if there's left in the chain
    if(task->next)
    {
      mg_loop_table_column_expansion(arena, strexpr, info, task->next, out);
    }
    
    //- rjf: if this is the last task in the chain, perform expansion
    else
    {
      String8List expansion_strs = {0};
      U64 start = 0;
      for(U64 char_idx = 0; char_idx <= strexpr.size;)
      {
        // rjf: push plain text parts of strexpr
        if(char_idx == strexpr.size || strexpr.str[char_idx] == '$')
        {
          String8 plain_text_substr = str8_substr(strexpr, r1u64(start, char_idx));
          start = char_idx;
          if(plain_text_substr.size != 0)
          {
            str8_list_push(arena, &expansion_strs, plain_text_substr);
          }
        }
        
        // rjf: handle expansion expression
        if(strexpr.str[char_idx] == '$')
        {
          String8 string = str8_skip(strexpr, char_idx+1);
          Rng1U64 expr_range = {0};
          S64 paren_nest = 0;
          for(U64 idx = 0; idx < string.size; idx += 1)
          {
            if(string.str[idx] == '(')
            {
              paren_nest += 1;
              if(paren_nest == 1)
              {
                expr_range.min = idx;
              }
            }
            if(string.str[idx] == ')')
            {
              paren_nest -= 1;
              if(paren_nest == 0)
              {
                expr_range.max = idx+1;
                break;
              }
            }
          }
          String8 expr_string = str8_substr(string, expr_range);
          MD_TokenizeResult expr_tokenize = md_tokenize_from_text(scratch.arena, expr_string);
          MD_ParseResult expr_base_parse = md_parse_from_text_tokens(scratch.arena, str8_lit(""), expr_string, expr_tokenize.tokens);
          MG_StrExprParseResult expr_parse = mg_str_expr_parse_from_root(scratch.arena, expr_base_parse.root->first);
          mg_eval_table_expand_expr__string(arena, expr_parse.root, info, &expansion_strs);
          char_idx = start = char_idx + 1 + expr_range.max;
        }
        else
        {
          char_idx += 1;
        }
      }
      String8 expansion_str = str8_list_join(arena, &expansion_strs, 0);
      if(expansion_str.size != 0)
      {
        str8_list_push(arena, out, expansion_str);
      }
    }
  }
  
  scratch_end(scratch);
}

internal String8List
mg_string_list_from_table_gen(Arena *arena, MG_Map grid_name_map, MG_Map grid_column_desc_map, String8 fallback, MD_Node *gen)
{
  String8List result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  if(md_node_is_nil(gen->first) && gen->string.size != 0)
  {
    str8_list_push(arena, &result, gen->string);
    str8_list_push(arena, &result, str8_lit("\n"));
  }
  else for(MD_EachNode(strexpr_node, gen->first))
  {
    // rjf: build task list
    MG_TableExpandTask *first_task = 0;
    MG_TableExpandTask *last_task = 0;
    for(MD_EachNode(tag, strexpr_node->first_tag))
    {
      if(str8_match(tag->string, str8_lit("expand"), 0))
      {
        // rjf: grab args for this expansion
        MD_Node *table_name_node = md_child_from_index(tag, 0);
        MD_Node *expand_label_node = md_child_from_index(tag, 1);
        String8 table_name = table_name_node->string;
        String8 expand_label = expand_label_node->string;
        
        // rjf: lookup table / column descriptions
        MG_NodeGrid *grid = mg_map_ptr_from_string(&grid_name_map, table_name);
        MG_ColumnDescArray *column_descs = mg_map_ptr_from_string(&grid_column_desc_map, table_name);
        
        // rjf: figure out row count
        U64 grid_row_count = 0;
        if(grid != 0)
        {
          grid_row_count = grid->cells.count / grid->y_stride;
        }
        
        // rjf: push task for this expansion
        if(grid != 0)
        {
          MG_TableExpandTask *task = push_array(scratch.arena, MG_TableExpandTask, 1);
          task->expansion_label = expand_label;
          task->grid = grid;
          task->column_descs = *column_descs;
          task->count = grid_row_count;
          task->idx = 0;
          SLLQueuePush(first_task, last_task, task);
        }
      }
    }
    
    // rjf: do expansion generation, OR just push this string if we have no expansions
    {
      MG_TableExpandInfo info = {first_task, fallback};
      if(first_task != 0)
      {
        mg_loop_table_column_expansion(arena, strexpr_node->string, &info, first_task, &result);
      }
      else
      {
        str8_list_push(arena, &result, strexpr_node->string);
      }
    }
  }
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Layer Lookup Functions

internal String8
mg_layer_key_from_path(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  U64 src_folder_pos = 0;
  for(U64 next_src_folder_pos = 0;
      next_src_folder_pos < path.size;
      next_src_folder_pos = str8_find_needle(path, next_src_folder_pos+1, str8_lit("src"), 0))
  {
    src_folder_pos = next_src_folder_pos;
  }
  String8List path_parts = str8_split_path(scratch.arena, str8_chop_last_slash(str8_skip(path, src_folder_pos+4)));
  StringJoin join = {0};
  join.sep = str8_lit("/");
  String8 key = str8_list_join(mg_arena, &path_parts, &join);
  scratch_end(scratch);
  return key;
}

internal MG_Layer *
mg_layer_from_key(String8 key)
{
  U64 hash = mg_hash_from_string(key);
  U64 slot_idx = hash%mg_state->slots_count;
  MG_LayerSlot *slot = &mg_state->slots[slot_idx];
  MG_Layer *layer = 0;
  for(MG_LayerNode *n = slot->first; n != 0; n = n->next)
  {
    if(str8_match(n->v.key, key, 0))
    {
      layer = &n->v;
      break;
    }
  }
  if(layer == 0)
  {
    MG_LayerNode *n = push_array(mg_arena, MG_LayerNode, 1);
    SLLQueuePush(slot->first, slot->last, n);
    n->v.key = push_str8_copy(mg_arena, key);
    layer = &n->v;
  }
  return layer;
}
