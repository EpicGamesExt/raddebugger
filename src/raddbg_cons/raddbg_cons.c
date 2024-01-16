// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//- "Public Facing" Cons API

//- init
static CONS_Root*
cons_root_new(CONS_RootParams *params){
  Arena *arena = arena_alloc__sized(GB(64), MB(256));
  CONS_Root *result = push_array(arena, CONS_Root, 1);
  result->arena = arena;
  
  // fill in root parameters
  {
    result->addr_size = params->addr_size;
  }
  
  // setup singular types
  {
    result->nil_type = cons__type_new(result);
    result->variadic_type = cons__type_new(result);
    result->variadic_type->kind = RADDBG_TypeKind_Variadic;
    
    // references to "handled nil type" should be emitted as
    // references to nil - but should not generate error
    // messages when they are detected - they are expected!
    Assert(result->nil_type->idx == result->handled_nil_type.idx);
  }
  
  // setup a null scope
  {
    CONS_Scope *scope = push_array(result->arena, CONS_Scope, 1);
    SLLQueuePush_N(result->first_scope, result->last_scope, scope, next_order);
    result->scope_count += 1;
  }
  
  // rjf: setup null UDT
  {
    cons__type_udt_from_any_type(result, result->nil_type);
  }
  
  // initialize maps
  {
#define BKTCOUNT(x) ((x)?(u64_up_to_pow2(x)):(128))
    
    cons__u64toptr_init(arena, &result->unit_map, BKTCOUNT(params->bucket_count_units));
    cons__u64toptr_init(arena, &result->symbol_map, BKTCOUNT(params->bucket_count_symbols));
    cons__u64toptr_init(arena, &result->scope_map, BKTCOUNT(params->bucket_count_scopes));
    cons__u64toptr_init(arena, &result->local_map, BKTCOUNT(params->bucket_count_locals));
    cons__u64toptr_init(arena, &result->type_from_id_map, BKTCOUNT(params->bucket_count_types));
    
#undef BKTCOUNT
  }
  
  return(result);
}

static void
cons_root_release(CONS_Root *root){
  arena_release(root->arena);
}

//- baking
static void
cons_bake_file(Arena *arena, CONS_Root *root, String8List *out){
  ProfBegin("cons_bake_file");
  str8_serial_begin(arena, out);
  
  // setup cons helpers
  CONS__DSections dss = {0};
  cons__dsection(arena, &dss, 0, 0, RADDBG_DataSectionTag_NULL);
  
  CONS__BakeCtx *bctx = cons__bake_ctx_begin();
  
  ////////////////////////////////
  // MAIN PART: allocating and filling out sections of the file
  
  // top level info
  RADDBG_TopLevelInfo *tli = push_array(arena, RADDBG_TopLevelInfo, 1);
  {
    CONS_TopLevelInfo *cons_tli = &root->top_level_info;
    tli->architecture = cons_tli->architecture;
    tli->exe_name_string_idx = cons__string(bctx, cons_tli->exe_name);
    tli->exe_hash = cons_tli->exe_hash;
    tli->voff_max = cons_tli->voff_max;
  }
  cons__dsection(arena, &dss, tli, sizeof(*tli), RADDBG_DataSectionTag_TopLevelInfo);
  
  // binary sections array
  {
    U32 count = root->binary_section_count;
    RADDBG_BinarySection *sections = push_array(arena, RADDBG_BinarySection, count);
    RADDBG_BinarySection *dsec = sections;
    for (CONS_BinarySection *ssec = root->binary_section_first;
         ssec != 0;
         ssec = ssec->next, dsec += 1){
      dsec->name_string_idx = cons__string(bctx, ssec->name);
      dsec->flags      = ssec->flags;
      dsec->voff_first = ssec->voff_first;
      dsec->voff_opl   = ssec->voff_opl;
      dsec->foff_first = ssec->foff_first;
      dsec->foff_opl   = ssec->foff_opl;
    }
    cons__dsection(arena, &dss, sections, sizeof(*sections)*count, RADDBG_DataSectionTag_BinarySections);
  }
  
  // units array
  // * pass for per-unit information including:
  // * top-level unit information
  // * combining line info for whole unit
  {
    U32 count = root->unit_count;
    RADDBG_Unit *units = push_array(arena, RADDBG_Unit, count);
    RADDBG_Unit *dunit = units;
    for (CONS_Unit *sunit = root->unit_first;
         sunit != 0;
         sunit = sunit->next_order, dunit += 1){
      // strings & paths
      U32 unit_name = cons__string(bctx, sunit->unit_name);
      U32 cmp_name  = cons__string(bctx, sunit->compiler_name);
      
      U32 src_path     = cons__paths_idx_from_path(bctx, sunit->source_file);
      U32 obj_path     = cons__paths_idx_from_path(bctx, sunit->object_file);
      U32 archive_path = cons__paths_idx_from_path(bctx, sunit->archive_file);
      U32 build_path   = cons__paths_idx_from_path(bctx, sunit->build_path);
      
      dunit->unit_name_string_idx     = unit_name;
      dunit->compiler_name_string_idx = cmp_name;
      dunit->source_file_path_node    = src_path;
      dunit->object_file_path_node    = obj_path;
      dunit->archive_file_path_node   = archive_path;
      dunit->build_path_node          = build_path;
      dunit->language                 = sunit->language;
      
      // line info (voff -> file*line*col)
      CONS_LineSequenceNode *first_seq = sunit->line_seq_first;
      CONS__UnitLinesCombined *lines = cons__unit_combine_lines(arena, bctx, first_seq);
      
      U32 line_count = lines->line_count;
      if (line_count > 0){
        dunit->line_info_voffs_data_idx =
          cons__dsection(arena, &dss, lines->voffs, sizeof(U64)*(line_count + 1),
                         RADDBG_DataSectionTag_LineInfoVoffs);
        dunit->line_info_data_idx =
          cons__dsection(arena, &dss, lines->lines, sizeof(RADDBG_Line)*line_count,
                         RADDBG_DataSectionTag_LineInfoData);
        if (lines->cols != 0){
          dunit->line_info_col_data_idx =
            cons__dsection(arena, &dss, lines->cols, sizeof(RADDBG_Column)*line_count,
                           RADDBG_DataSectionTag_LineInfoColumns);
        }
        dunit->line_info_count = line_count;
      }
    }
    
    cons__dsection(arena, &dss, units, sizeof(*units)*count, RADDBG_DataSectionTag_Units);
  }
  
  // source file line info baking
  // * pass for "source_combine_line" for each source file -
  // * can only be run after a pass that does "unit_combine_lines" for each unit.
  for (CONS__SrcNode *src_node = bctx->tree->src_first;
       src_node != 0;
       src_node = src_node->next){
    CONS__LineMapFragment *first_fragment = src_node->first_fragment;
    CONS__SrcLinesCombined *lines = cons__source_combine_lines(arena, first_fragment);
    U32 line_count = lines->line_count;
    
    if (line_count > 0){
      src_node->line_map_count = line_count;
      
      src_node->line_map_nums_data_idx =
        cons__dsection(arena, &dss, lines->line_nums, sizeof(*lines->line_nums)*line_count,
                       RADDBG_DataSectionTag_LineMapNumbers);
      
      src_node->line_map_range_data_idx =
        cons__dsection(arena, &dss, lines->line_ranges, sizeof(*lines->line_ranges)*(line_count + 1),
                       RADDBG_DataSectionTag_LineMapRanges);
      
      src_node->line_map_voff_data_idx =
        cons__dsection(arena, &dss, lines->voffs, sizeof(*lines->voffs)*lines->voff_count,
                       RADDBG_DataSectionTag_LineMapVoffs);
    }
  }
  
  // source file name mapping
  {
    CONS__NameMap* map = cons__name_map_for_kind(root, RADDBG_NameMapKind_NormalSourcePaths);
    for (CONS__SrcNode *src_node = bctx->tree->src_first;
         src_node != 0;
         src_node = src_node->next){
      if (src_node->idx != 0){
        cons__name_map_add_pair(root, map, src_node->normal_full_path, src_node->idx);
      }
    }
  }
  
  // unit vmap baking
  {
    CONS__VMap *vmap = cons__vmap_from_unit_ranges(arena,
                                                   root->unit_vmap_range_first,
                                                   root->unit_vmap_range_count);
    
    U64 vmap_size = sizeof(*vmap->vmap)*(vmap->count + 1);
    cons__dsection(arena, &dss, vmap->vmap, vmap_size, RADDBG_DataSectionTag_UnitVmap);
  }
  
  // type info baking
  {
    CONS__TypeData *types = cons__type_data_combine(arena, root, bctx);
    
    U64 type_nodes_size = sizeof(*types->type_nodes)*types->type_node_count;
    cons__dsection(arena, &dss, types->type_nodes, type_nodes_size, RADDBG_DataSectionTag_TypeNodes);
    
    U64 udt_size = sizeof(*types->udts)*types->udt_count;
    cons__dsection(arena, &dss, types->udts, udt_size, RADDBG_DataSectionTag_UDTs);
    
    U64 member_size = sizeof(*types->members)*types->member_count;
    cons__dsection(arena, &dss, types->members, member_size, RADDBG_DataSectionTag_Members);
    
    U64 enum_member_size = sizeof(*types->enum_members)*types->enum_member_count;
    cons__dsection(arena, &dss, types->enum_members, enum_member_size, RADDBG_DataSectionTag_EnumMembers);
  }
  
  // symbol info baking
  {
    CONS__SymbolData *symbol_data = cons__symbol_data_combine(arena, root, bctx);
    
    U64 global_variables_size =
      sizeof(*symbol_data->global_variables)*symbol_data->global_variable_count;
    cons__dsection(arena, &dss, symbol_data->global_variables, global_variables_size,
                   RADDBG_DataSectionTag_GlobalVariables);
    
    CONS__VMap *global_vmap = symbol_data->global_vmap;
    U64 global_vmap_size = sizeof(*global_vmap->vmap)*(global_vmap->count + 1);
    cons__dsection(arena, &dss, global_vmap->vmap, global_vmap_size,
                   RADDBG_DataSectionTag_GlobalVmap);
    
    U64 thread_variables_size =
      sizeof(*symbol_data->thread_variables)*symbol_data->thread_variable_count;
    cons__dsection(arena, &dss, symbol_data->thread_variables, thread_variables_size,
                   RADDBG_DataSectionTag_ThreadVariables);
    
    U64 procedures_size = sizeof(*symbol_data->procedures)*symbol_data->procedure_count;
    cons__dsection(arena, &dss, symbol_data->procedures, procedures_size,
                   RADDBG_DataSectionTag_Procedures);
    
    U64 scopes_size = sizeof(*symbol_data->scopes)*symbol_data->scope_count;
    cons__dsection(arena, &dss, symbol_data->scopes, scopes_size, RADDBG_DataSectionTag_Scopes);
    
    U64 scope_voffs_size = sizeof(*symbol_data->scope_voffs)*symbol_data->scope_voff_count;
    cons__dsection(arena, &dss, symbol_data->scope_voffs, scope_voffs_size,
                   RADDBG_DataSectionTag_ScopeVoffData);
    
    CONS__VMap *scope_vmap = symbol_data->scope_vmap;
    U64 scope_vmap_size = sizeof(*scope_vmap->vmap)*(scope_vmap->count + 1);
    cons__dsection(arena, &dss, scope_vmap->vmap, scope_vmap_size, RADDBG_DataSectionTag_ScopeVmap);
    
    U64 local_size = sizeof(*symbol_data->locals)*symbol_data->local_count;
    cons__dsection(arena, &dss, symbol_data->locals, local_size, RADDBG_DataSectionTag_Locals);
    
    U64 location_blocks_size =
      sizeof(*symbol_data->location_blocks)*symbol_data->location_block_count;
    cons__dsection(arena, &dss, symbol_data->location_blocks, location_blocks_size,
                   RADDBG_DataSectionTag_LocationBlocks);
    
    U64 location_data_size = symbol_data->location_data_size;
    cons__dsection(arena, &dss, symbol_data->location_data, location_data_size,
                   RADDBG_DataSectionTag_LocationData);
  }
  
  // name map baking
  {
    U32 name_map_count = 0;
    for (U32 i = 0; i < RADDBG_NameMapKind_COUNT; i += 1){
      if (root->name_maps[i] != 0){
        name_map_count += 1;
      }
    }
    
    RADDBG_NameMap *name_maps = push_array(arena, RADDBG_NameMap, name_map_count);
    
    RADDBG_NameMap *name_map_ptr = name_maps;
    for (U32 i = 0; i < RADDBG_NameMapKind_COUNT; i += 1){
      CONS__NameMap *map = root->name_maps[i];
      if (map != 0){
        CONS__NameMapBaked *baked = cons__name_map_bake(arena, root, bctx, map);
        
        name_map_ptr->kind = i;
        name_map_ptr->bucket_data_idx =
          cons__dsection(arena, &dss, baked->buckets, sizeof(*baked->buckets)*baked->bucket_count,
                         RADDBG_DataSectionTag_NameMapBuckets);
        name_map_ptr->node_data_idx =
          cons__dsection(arena, &dss, baked->nodes, sizeof(*baked->nodes)*baked->node_count,
                         RADDBG_DataSectionTag_NameMapNodes);
        name_map_ptr += 1;
      }
    }
    
    cons__dsection(arena, &dss, name_maps, sizeof(*name_maps)*name_map_count,
                   RADDBG_DataSectionTag_NameMaps);
  }
  
  ////////////////////////////////
  // LATE PART: baking loose structures and creating final layout
  
  // generate data sections for file paths
  {
    U32 count = bctx->tree->count;
    RADDBG_FilePathNode *nodes = push_array(arena, RADDBG_FilePathNode, count);
    
    RADDBG_FilePathNode *out_node = nodes;
    for (CONS__PathNode *node = bctx->tree->first;
         node != 0;
         node = node->next_order, out_node += 1){
      out_node->name_string_idx = cons__string(bctx, node->name);
      if (node->parent != 0){
        out_node->parent_path_node = node->parent->idx;
      }
      if (node->first_child != 0){
        out_node->first_child = node->first_child->idx;
      }
      if (node->next_sibling != 0){
        out_node->next_sibling = node->next_sibling->idx;
      }
      if (node->src_file != 0){
        out_node->source_file_idx = node->src_file->idx;
      }
    }
    
    cons__dsection(arena, &dss, nodes, sizeof(*nodes)*count, RADDBG_DataSectionTag_FilePathNodes);
  }
  
  // generate data sections for files
  {
    U32 count = bctx->tree->src_count;
    RADDBG_SourceFile *src_files = push_array(arena, RADDBG_SourceFile, count);
    
    RADDBG_SourceFile *out_src_file = src_files;
    for (CONS__SrcNode *node = bctx->tree->src_first;
         node != 0;
         node = node->next, out_src_file += 1){
      out_src_file->file_path_node_idx = node->path_node->idx;
      out_src_file->normal_full_path_string_idx = cons__string(bctx, node->normal_full_path);
      out_src_file->line_map_nums_data_idx = node->line_map_nums_data_idx;
      out_src_file->line_map_range_data_idx = node->line_map_range_data_idx;
      out_src_file->line_map_count = node->line_map_count;
      out_src_file->line_map_voff_data_idx = node->line_map_voff_data_idx;
    }
    
    cons__dsection(arena, &dss, src_files, sizeof(*src_files)*count, RADDBG_DataSectionTag_SourceFiles);
  }
  
  // generate data sections for strings
  {
    U32 *str_offs = push_array_no_zero(arena, U32, bctx->strs.count + 1);
    
    U32 off_cursor = 0;
    {
      U32 *off_ptr = str_offs;
      *off_ptr = 0;
      off_ptr += 1;
      for (CONS__StringNode *node = bctx->strs.order_first;
           node != 0;
           node = node->order_next){
        off_cursor += node->str.size;
        *off_ptr = off_cursor;
        off_ptr += 1;
      }
    }
    
    U8 *buf = push_array(arena, U8, off_cursor);
    {
      U8 *ptr = buf;
      for (CONS__StringNode *node = bctx->strs.order_first;
           node != 0;
           node = node->order_next){
        MemoryCopy(ptr, node->str.str, node->str.size);
        ptr += node->str.size;
      }
    }
    
    cons__dsection(arena, &dss, str_offs, sizeof(*str_offs)*(bctx->strs.count + 1),
                   RADDBG_DataSectionTag_StringTable);
    cons__dsection(arena, &dss, buf, off_cursor, RADDBG_DataSectionTag_StringData);
  }
  
  // generate data sections for index runs
  {
    U32 *idx_data = push_array_no_zero(arena, U32, bctx->idxs.idx_count);
    
    {
      U32 *out_ptr = idx_data;
      U32 *opl = out_ptr + bctx->idxs.idx_count;
      CONS__IdxRunNode *node = bctx->idxs.order_first;
      for (;node != 0 && out_ptr < opl;
           node = node->order_next){
        MemoryCopy(out_ptr, node->idx_run, sizeof(*node->idx_run)*node->count);
        out_ptr += node->count;
      }
      // both iterators should reach the end at the same time
      Assert(node == 0);
      Assert(out_ptr == opl);
    }
    
    cons__dsection(arena, &dss, idx_data, sizeof(*idx_data)*bctx->idxs.idx_count,
                   RADDBG_DataSectionTag_IndexRuns);
  }
  
  // layout
  // * the header and data section table have to be initialized "out of order"
  // * so that the rest of the system can avoid this tricky order-layout interdependence stuff
  RADDBG_Header *header = push_array(arena, RADDBG_Header, 1);
  RADDBG_DataSection *dstable = push_array(arena, RADDBG_DataSection, dss.count);
  str8_serial_push_align(arena, out, 8);
  U64 header_off = out->total_size;
  str8_list_push(arena, out, str8_struct(header));
  str8_serial_push_align(arena, out, 8);
  U64 data_section_off = out->total_size;
  str8_list_push(arena, out, str8((U8 *)dstable, sizeof(*dstable)*dss.count));
  {
    header->magic = RADDBG_MAGIC_CONSTANT;
    header->encoding_version = RADDBG_ENCODING_VERSION;
    header->data_section_off = data_section_off;
    header->data_section_count = dss.count;
  }
  {
    U64 test_dss_count = 0;
    for (CONS__DSectionNode *node = dss.first;
         node != 0;
         node = node->next){
      test_dss_count += 1;
    }
    Assert(test_dss_count == dss.count);
    
    RADDBG_DataSection *ptr = dstable;
    for (CONS__DSectionNode *node = dss.first;
         node != 0;
         node = node->next, ptr += 1){
      U64 data_section_offset = 0;
      if(node->size != 0)
      {
        str8_serial_push_align(arena, out, 8);
        data_section_offset = out->total_size;
        str8_list_push(arena, out, str8((U8 *)node->data, node->size));
      }
      ptr->tag = node->tag;
      ptr->encoding = RADDBG_DataSectionEncoding_Unpacked;
      ptr->off = data_section_offset;
      ptr->encoded_size = node->size;
      ptr->unpacked_size = node->size;
    }
    Assert(ptr == dstable + dss.count);
  }
  
  cons__bake_ctx_release(bctx);
  ProfEnd();
}


//- errors
static void
cons_errorf(CONS_Root *root, char *fmt, ...){
  ProfBeginFunction();
  CONS_Error *error = push_array(root->arena, CONS_Error, 1);
  SLLQueuePush(root->errors.first, root->errors.last, error);
  root->errors.count += 1;
  
  va_list args;
  va_start(args, fmt);
  String8 str = push_str8fv(root->arena, fmt, args);
  va_end(args);
  
  error->msg = str;
  ProfEnd();
}

static CONS_Error*
cons_get_first_error(CONS_Root *root){
  return(root->errors.first);
}


//- information declaration

// top level info

static void
cons_set_top_level_info(CONS_Root *root, CONS_TopLevelInfo *tli){
  if (root->top_level_info_is_set){
    // TODO(allen): API error
  }
  else{
    MemoryCopyStruct(&root->top_level_info, tli);
    root->top_level_info_is_set = 1;
  }
}

// binary sections

static void
cons_add_binary_section(CONS_Root *root, String8 name, RADDBG_BinarySectionFlags flags,
                        U64 voff_first, U64 voff_opl, U64 foff_first, U64 foff_opl){
  CONS_BinarySection *sec = push_array(root->arena, CONS_BinarySection, 1);
  SLLQueuePush(root->binary_section_first, root->binary_section_last, sec);
  root->binary_section_count += 1;
  
  sec->name  = name;
  sec->flags = flags;
  sec->voff_first = voff_first;
  sec->voff_opl   = voff_opl;
  sec->foff_first = foff_first;
  sec->foff_opl   = foff_opl;
}

// units

static CONS_Unit*
cons_unit_handle_from_user_id(CONS_Root *root, U64 unit_user_id){
  CONS__U64ToPtrLookup lookup = {0};
  cons__u64toptr_lookup(&root->unit_map, unit_user_id, &lookup);
  
  CONS_Unit *result = 0;
  if (lookup.match != 0){
    result = (CONS_Unit*)lookup.match;
  }
  else{
    result = push_array(root->arena, CONS_Unit, 1);
    result->idx = root->unit_count;
    SLLQueuePush_N(root->unit_first, root->unit_last, result, next_order);
    root->unit_count += 1;
    cons__u64toptr_insert(root->arena, &root->unit_map, unit_user_id, &lookup, result);
  }
  
  return(result);
}

static void
cons_unit_set_info(CONS_Root *root, CONS_Unit *unit, CONS_UnitInfo *info){
  if (unit->info_is_set){
    // TODO(allen): API error
  }
  else{
    unit->info_is_set = 1;
    unit->unit_name = push_str8_copy(root->arena, info->unit_name);
    unit->compiler_name = push_str8_copy(root->arena, info->compiler_name);
    unit->source_file = push_str8_copy(root->arena, info->source_file);
    unit->object_file = push_str8_copy(root->arena, info->object_file);
    unit->archive_file = push_str8_copy(root->arena, info->archive_file);
    unit->build_path = push_str8_copy(root->arena, info->build_path);
    unit->language = info->language;
  }
}

static void
cons_unit_add_line_sequence(CONS_Root *root, CONS_Unit *unit, CONS_LineSequence *line_sequence){
  CONS_LineSequenceNode *node = push_array(root->arena, CONS_LineSequenceNode, 1);
  SLLQueuePush(unit->line_seq_first, unit->line_seq_last, node);
  unit->line_seq_count += 1;
  
  node->line_seq.file_name = push_str8_copy(root->arena, line_sequence->file_name);
  
  node->line_seq.voffs = push_array(root->arena, U64, line_sequence->line_count + 1);
  MemoryCopy(node->line_seq.voffs, line_sequence->voffs, sizeof(U64)*(line_sequence->line_count + 1));
  
  node->line_seq.line_nums = push_array(root->arena, U32, line_sequence->line_count);
  MemoryCopy(node->line_seq.line_nums, line_sequence->line_nums, sizeof(U32)*line_sequence->line_count);
  
  if (line_sequence->col_nums != 0){
    node->line_seq.col_nums = push_array(root->arena, U16, line_sequence->line_count);
    MemoryCopy(node->line_seq.col_nums, line_sequence->col_nums, sizeof(U16)*line_sequence->line_count);
  }
  
  node->line_seq.line_count = line_sequence->line_count;
}

static void
cons_unit_vmap_add_range(CONS_Root *root, CONS_Unit *unit, U64 first, U64 opl){
  CONS_UnitVMapRange *node = push_array(root->arena, CONS_UnitVMapRange, 1);
  SLLQueuePush(root->unit_vmap_range_first, root->unit_vmap_range_last, node);
  root->unit_vmap_range_count += 1;
  
  node->unit = unit;
  node->first = first;
  node->opl = opl;
}

// types

static CONS_Type*
cons_type_from_id(CONS_Root *root, U64 type_user_id){
  CONS__U64ToPtrLookup lookup = {0};
  cons__u64toptr_lookup(&root->type_from_id_map, type_user_id, &lookup);
  
  CONS_Type *result = (CONS_Type*)lookup.match;
  return(result);
}

static CONS_Reservation*
cons_type_reserve_id(CONS_Root *root, U64 type_user_id){
  CONS__U64ToPtrLookup lookup = {0};
  cons__u64toptr_lookup(&root->type_from_id_map, type_user_id, &lookup);
  
  CONS_Reservation *result = 0;
  if (lookup.match == 0){
    cons__u64toptr_insert(root->arena, &root->type_from_id_map, type_user_id, &lookup, root->nil_type);
    void **slot = &lookup.fill_node->ptr[lookup.fill_k];
    result = (CONS_Reservation*)slot;
  }
  
  return(result);
}

static void
cons_type_fill_id(CONS_Root *root, CONS_Reservation *res, CONS_Type *type){
  if (res != 0 && type != 0){
    *(void**)res = type;
  }
}

static B32
cons_type_is_unhandled_nil(CONS_Root *root, CONS_Type *type){
  B32 result = (type->kind == RADDBG_TypeKind_NULL &&
                type != &root->handled_nil_type);
  return(result);
}

static CONS_Type*
cons_type_handled_nil(CONS_Root *root){
  return(&root->handled_nil_type);
}

static CONS_Type*
cons_type_nil(CONS_Root *root){
  return(root->nil_type);
}

static CONS_Type*
cons_type_variadic(CONS_Root *root){
  return(root->variadic_type);
}

static CONS_Type*
cons_type_basic(CONS_Root *root, RADDBG_TypeKind type_kind, String8 name){
  CONS_Type *result = root->nil_type;
  
  if (!(RADDBG_TypeKind_FirstBuiltIn <= type_kind &&
        type_kind <= RADDBG_TypeKind_LastBuiltIn)){
    // TODO(allen): API error
  }
  else{
    Temp scratch = scratch_begin(0, 0);
    
    // setup construct buffer
    U64 buf_size = sizeof(CONS_TypeConstructKind) + sizeof(type_kind) + name.size;
    U8 *buf = push_array(scratch.arena, U8, buf_size);
    {
      U8 *ptr = buf;
      // "basic"
      *(CONS_TypeConstructKind*)ptr = CONS_TypeConstructKind_Basic;
      ptr += sizeof(CONS_TypeConstructKind);
      // type_kind
      MemoryCopy(ptr, &type_kind, sizeof(type_kind));
      ptr += sizeof(type_kind);
      // name
      MemoryCopy(ptr, name.str, name.size);
      ptr += name.size;
    }
    
    // check for duplicate construct
    String8 blob = str8(buf, buf_size);
    U64 blob_hash = raddbg_hash(buf, buf_size);
    void *lookup_ptr = cons__str8toptr_lookup(&root->construct_map, blob, blob_hash);
    result = (CONS_Type*)lookup_ptr;
    if (result == 0){
      // calculate size
      U32 byte_size = raddbg_size_from_basic_type_kind(type_kind);
      if (byte_size == 0xFFFFFFFF){
        byte_size = root->addr_size;
      }
      
      // setup new node
      result = cons__type_new(root);
      result->kind = type_kind;
      result->name = push_str8_copy(root->arena, name);
      result->byte_size = byte_size;
      
      // save in construct map
      cons__str8toptr_insert(root->arena, &root->construct_map, blob, blob_hash, result);
      
      // save in name map
      {
        CONS__NameMap *map = cons__name_map_for_kind(root, RADDBG_NameMapKind_Types);
        cons__name_map_add_pair(root, map, result->name, result->idx);
      }
    }
    
    scratch_end(scratch);
  }
  
  Assert(result != 0);
  return(result);
}

static CONS_Type*
cons_type_modifier(CONS_Root *root, CONS_Type *direct_type, RADDBG_TypeModifierFlags flags){
  ProfBeginFunction();
  CONS_Type *result = root->nil_type;
  
  {
    Temp scratch = scratch_begin(0, 0);
    
    // setup construct buffer
    U64 buf_size = sizeof(CONS_TypeConstructKind) + sizeof(flags) + sizeof(direct_type->idx);
    U8 *buf = push_array(scratch.arena, U8, buf_size);
    {
      U8 *ptr = buf;
      // "modifier"
      *(CONS_TypeConstructKind*)ptr = CONS_TypeConstructKind_Modifier;
      ptr += sizeof(CONS_TypeConstructKind);
      // flags
      MemoryCopy(ptr, &flags, sizeof(flags));
      ptr += sizeof(flags);
      // direct_type->idx
      MemoryCopy(ptr, &direct_type->idx, sizeof(direct_type->idx));
      ptr += sizeof(direct_type->idx);
    }
    
    // check for duplicate construct
    String8 blob = str8(buf, buf_size);
    U64 blob_hash = raddbg_hash(buf, buf_size);
    void *lookup_ptr = cons__str8toptr_lookup(&root->construct_map, blob, blob_hash);
    result = (CONS_Type*)lookup_ptr;
    if (result == 0){
      
      // setup new node
      result = cons__type_new(root);
      result->kind = RADDBG_TypeKind_Modifier;
      result->flags = flags;
      result->byte_size = direct_type->byte_size;
      result->direct_type = direct_type;
      
      // save in construct map
      cons__str8toptr_insert(root->arena, &root->construct_map, blob, blob_hash, result);
    }
    
    scratch_end(scratch);
  }
  
  Assert(result != 0);
  ProfEnd();
  return(result);
}

static CONS_Type*
cons_type_bitfield(CONS_Root *root, CONS_Type *direct_type, U32 bit_off, U32 bit_count){
  CONS_Type *result = root->nil_type;
  
  {
    Temp scratch = scratch_begin(0, 0);
    
    // setup construct buffer
    U64 buf_size = sizeof(CONS_TypeConstructKind) + sizeof(direct_type->idx) + sizeof(U32)*2;
    U8 *buf = push_array(scratch.arena, U8, buf_size);
    {
      U8 *ptr = buf;
      // "bitfield"
      *(CONS_TypeConstructKind*)ptr = CONS_TypeConstructKind_Bitfield;
      ptr += sizeof(CONS_TypeConstructKind);
      // direct_type->idx
      MemoryCopy(ptr, &direct_type->idx, sizeof(direct_type->idx));
      ptr += sizeof(direct_type->idx);
      // bit_off
      MemoryCopy(ptr, &bit_off, sizeof(bit_off));
      ptr += sizeof(bit_off);
      // bit_count
      MemoryCopy(ptr, &bit_count, sizeof(bit_count));
      ptr += sizeof(bit_count);
    }
    
    // check for duplicate construct
    String8 blob = str8(buf, buf_size);
    U64 blob_hash = raddbg_hash(buf, buf_size);
    void *lookup_ptr = cons__str8toptr_lookup(&root->construct_map, blob, blob_hash);
    result = (CONS_Type*)lookup_ptr;
    if (result == 0){
      
      // setup new node
      result = cons__type_new(root);
      result->kind = RADDBG_TypeKind_Bitfield;
      result->byte_size = direct_type->byte_size;
      result->off = bit_off;
      result->count = bit_count;
      result->direct_type = direct_type;
      
      // save in construct map
      cons__str8toptr_insert(root->arena, &root->construct_map, blob, blob_hash, result);
    }
    
    scratch_end(scratch);
  }
  
  Assert(result != 0);
  return(result);
}

static CONS_Type*
cons_type_pointer(CONS_Root *root, CONS_Type *direct_type, RADDBG_TypeKind ptr_type_kind){
  ProfBeginFunction();
  CONS_Type *result = root->nil_type;
  
  if (!(ptr_type_kind == RADDBG_TypeKind_Ptr ||
        ptr_type_kind == RADDBG_TypeKind_LRef ||
        ptr_type_kind == RADDBG_TypeKind_RRef)){
    // TODO(allen): API error
  }
  else{
    Temp scratch = scratch_begin(0, 0);
    
    // setup construct buffer
    U64 buf_size = sizeof(CONS_TypeConstructKind) + sizeof(ptr_type_kind) + sizeof(direct_type->idx);
    U8 *buf = push_array(scratch.arena, U8, buf_size);
    {
      U8 *ptr = buf;
      // "pointer"
      *(CONS_TypeConstructKind*)ptr = CONS_TypeConstructKind_Pointer;
      ptr += sizeof(CONS_TypeConstructKind);
      // type_kind
      MemoryCopy(ptr, &ptr_type_kind, sizeof(ptr_type_kind));
      ptr += sizeof(ptr_type_kind);
      // direct_type->idx
      MemoryCopy(ptr, &direct_type->idx, sizeof(direct_type->idx));
      ptr += sizeof(direct_type->idx);
    }
    
    // check for duplicate construct
    String8 blob = str8(buf, buf_size);
    U64 blob_hash = raddbg_hash(buf, buf_size);
    void *lookup_ptr = cons__str8toptr_lookup(&root->construct_map, blob, blob_hash);
    result = (CONS_Type*)lookup_ptr;
    if (result == 0){
      
      // setup new node
      result = cons__type_new(root);
      result->kind = ptr_type_kind;
      result->byte_size = root->addr_size;
      result->direct_type = direct_type;
      
      // save in construct map
      cons__str8toptr_insert(root->arena, &root->construct_map, blob, blob_hash, result);
    }
    
    scratch_end(scratch);
  }
  
  Assert(result != 0);
  ProfEnd();
  return(result);
}

static CONS_Type*
cons_type_array(CONS_Root *root, CONS_Type *direct_type, U64 count){
  CONS_Type *result = root->nil_type;
  
  {
    Temp scratch = scratch_begin(0, 0);
    
    // setup construct buffer
    U64 buf_size =
      sizeof(CONS_TypeConstructKind) + sizeof(direct_type->idx) + sizeof(count);
    U8 *buf = push_array(scratch.arena, U8, buf_size);
    {
      U8 *ptr = buf;
      // "array"
      *(CONS_TypeConstructKind*)ptr = CONS_TypeConstructKind_Array;
      ptr += sizeof(CONS_TypeConstructKind);
      // direct_type->idx
      MemoryCopy(ptr, &direct_type->idx, sizeof(direct_type->idx));
      ptr += sizeof(direct_type->idx);
      // count
      MemoryCopy(ptr, &count, sizeof(count));
      ptr += sizeof(count);
    }
    
    // check for duplicate construct
    String8 blob = str8(buf, buf_size);
    U64 blob_hash = raddbg_hash(buf, buf_size);
    void *lookup_ptr = cons__str8toptr_lookup(&root->construct_map, blob, blob_hash);
    result = (CONS_Type*)lookup_ptr;
    if (result == 0){
      
      // setup new node
      result = cons__type_new(root);
      result->kind = RADDBG_TypeKind_Array;
      result->count = count;
      result->direct_type = direct_type;
      result->byte_size = direct_type->byte_size*count;
      
      // save in construct map
      cons__str8toptr_insert(root->arena, &root->construct_map, blob, blob_hash, result);
    }
    
    scratch_end(scratch);
  }
  
  Assert(result != 0);
  return(result);
}

static CONS_Type*
cons_type_proc(CONS_Root *root, CONS_Type *return_type, CONS_TypeList *params){
  ProfBeginFunction();
  CONS_Type *result = root->nil_type;
  
  {
    Temp scratch = scratch_begin(0, 0);
    
    // setup construct buffer
    U64 buf_size = sizeof(CONS_TypeConstructKind) + sizeof(return_type->idx)*(1 + params->count);
    U8 *buf = push_array(scratch.arena, U8, buf_size);
    {
      U8 *ptr = buf;
      // "procedure"
      *(CONS_TypeConstructKind*)ptr = CONS_TypeConstructKind_Procedure;
      ptr += sizeof(CONS_TypeConstructKind);
      // ret_type->idx
      MemoryCopy(ptr, &return_type->idx, sizeof(return_type->idx));
      ptr += sizeof(return_type->idx);
      // (params ...)->idx
      for (CONS_TypeNode *node = params->first;
           node != 0;
           node = node->next){
        MemoryCopy(ptr, &node->type->idx, sizeof(node->type->idx));
        ptr += sizeof(node->type->idx);
      }
    }
    
    // check for duplicate construct
    String8 blob = str8(buf, buf_size);
    U64 blob_hash = raddbg_hash(buf, buf_size);
    void *lookup_ptr = cons__str8toptr_lookup(&root->construct_map, blob, blob_hash);
    result = (CONS_Type*)lookup_ptr;
    if (result == 0){
      
      // setup param buffer
      CONS_Type **param_types = push_array(root->arena, CONS_Type*, params->count);
      {
        CONS_Type **ptr = param_types;
        for (CONS_TypeNode *node = params->first;
             node != 0;
             node = node->next){
          *ptr = node->type;
          ptr += 1;
        }
      }
      
      // setup new node
      result = cons__type_new(root);
      result->kind = RADDBG_TypeKind_Function;
      result->byte_size = root->addr_size;
      result->count = params->count;
      result->direct_type = return_type;
      result->param_types = param_types;
      
      // save in construct map
      cons__str8toptr_insert(root->arena, &root->construct_map, blob, blob_hash, result);
    }
    
    scratch_end(scratch);
  }
  
  Assert(result != 0);
  ProfEnd();
  return(result);
}

static CONS_Type*
cons_type_method(CONS_Root *root, CONS_Type *this_type, CONS_Type *return_type, CONS_TypeList *params){
  ProfBeginFunction(0, 0);
  CONS_Type *result = root->nil_type;
  
  {
    Temp scratch = scratch_begin(0, 0);
    
    // setup construct buffer
    U64 buf_size =
      sizeof(CONS_TypeConstructKind) + sizeof(return_type->idx)*(2 + params->count);
    U8 *buf = push_array(scratch.arena, U8, buf_size);
    {
      U8 *ptr = buf;
      // "method"
      *(CONS_TypeConstructKind*)ptr = CONS_TypeConstructKind_Method;
      ptr += sizeof(CONS_TypeConstructKind);
      // ret_type->idx
      MemoryCopy(ptr, &return_type->idx, sizeof(return_type->idx));
      ptr += sizeof(return_type->idx);
      // this_type->idx
      MemoryCopy(ptr, &this_type->idx, sizeof(this_type->idx));
      ptr += sizeof(this_type->idx);
      // (params ...)->idx
      for (CONS_TypeNode *node = params->first;
           node != 0;
           node = node->next){
        MemoryCopy(ptr, &node->type->idx, sizeof(node->type->idx));
        ptr += sizeof(node->type->idx);
      }
    }
    
    // check for duplicate construct
    String8 blob = str8(buf, buf_size);
    U64 blob_hash = raddbg_hash(buf, buf_size);
    void *lookup_ptr = cons__str8toptr_lookup(&root->construct_map, blob, blob_hash);
    result = (CONS_Type*)lookup_ptr;
    if (result == 0){
      
      // setup param buffer
      CONS_Type **param_types = push_array(root->arena, CONS_Type*, params->count + 1);
      {
        CONS_Type **ptr = param_types;
        {
          *ptr = this_type;
          ptr += 1;
        }
        for (CONS_TypeNode *node = params->first;
             node != 0;
             node = node->next){
          *ptr = node->type;
          ptr += 1;
        }
      }
      
      // setup new node
      result = cons__type_new(root);
      result->kind = RADDBG_TypeKind_Method;
      result->byte_size = root->addr_size;
      result->count = params->count;
      result->direct_type = return_type;
      result->param_types = param_types;
      
      // save in construct map
      cons__str8toptr_insert(root->arena, &root->construct_map, blob, blob_hash, result);
    }
    
    scratch_end(scratch);
  }
  
  Assert(result != 0);
  ProfEnd();
  return(result);
}

static CONS_Type*
cons_type_udt(CONS_Root *root, RADDBG_TypeKind record_type_kind, String8 name, U64 size){
  CONS_Type *result = root->nil_type;
  
  if (!(record_type_kind == RADDBG_TypeKind_Struct ||
        record_type_kind == RADDBG_TypeKind_Class ||
        record_type_kind == RADDBG_TypeKind_Union)){
    // TODO(allen): API error
  }
  else{
    result = cons__type_new(root);
    result->kind = record_type_kind;
    result->byte_size = size;
    result->name = push_str8_copy(root->arena, name);
    
    // save in name map
    {
      CONS__NameMap *map = cons__name_map_for_kind(root, RADDBG_NameMapKind_Types);
      cons__name_map_add_pair(root, map, result->name, result->idx);
    }
  }
  
  return(result);
}

static CONS_Type*
cons_type_enum(CONS_Root *root, CONS_Type *direct_type, String8 name){
  CONS_Type *result = cons__type_new(root);
  result->kind = RADDBG_TypeKind_Enum;
  result->byte_size = direct_type->byte_size;
  result->name = push_str8_copy(root->arena, name);
  result->direct_type = direct_type;
  
  // save in name map
  {
    CONS__NameMap *map = cons__name_map_for_kind(root, RADDBG_NameMapKind_Types);
    cons__name_map_add_pair(root, map, result->name, result->idx);
  }
  
  return(result);
}

static CONS_Type*
cons_type_alias(CONS_Root *root, CONS_Type *direct_type, String8 name){
  CONS_Type *result = cons__type_new(root);
  result->kind = RADDBG_TypeKind_Alias;
  result->byte_size = direct_type->byte_size;
  result->name = push_str8_copy(root->arena, name);
  result->direct_type = direct_type;
  
  // save in name map
  {
    CONS__NameMap *map = cons__name_map_for_kind(root, RADDBG_NameMapKind_Types);
    cons__name_map_add_pair(root, map, result->name, result->idx);
  }
  
  return(result);
}

static CONS_Type*
cons_type_incomplete(CONS_Root *root, RADDBG_TypeKind type_kind, String8 name){
  CONS_Type *result = root->nil_type;
  
  if (!(type_kind == RADDBG_TypeKind_IncompleteStruct ||
        type_kind == RADDBG_TypeKind_IncompleteClass ||
        type_kind == RADDBG_TypeKind_IncompleteUnion ||
        type_kind == RADDBG_TypeKind_IncompleteEnum)){
    // TODO(allen): API error
  }
  else{
    result = cons__type_new(root);
    result->kind = type_kind;
    result->name = push_str8_copy(root->arena, name);
    
    // save in name map
    {
      CONS__NameMap *map = cons__name_map_for_kind(root, RADDBG_NameMapKind_Types);
      cons__name_map_add_pair(root, map, result->name, result->idx);
    }
  }
  
  return(result);
}

static void
cons_type_add_member_data_field(CONS_Root *root, CONS_Type *record_type,
                                String8 name, CONS_Type *mem_type, U32 off){
  CONS_TypeUDT *udt = cons__type_udt_from_record_type(root, record_type);
  if (udt != 0){
    CONS_TypeMember *member = push_array(root->arena, CONS_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    
    root->total_member_count += 1;
    
    member->kind = RADDBG_MemberKind_DataField;
    member->name = push_str8_copy(root->arena, name);
    member->type = mem_type;
    member->off = off;
  }
}

static void
cons_type_add_member_static_data(CONS_Root *root, CONS_Type *record_type,
                                 String8 name, CONS_Type *mem_type){
  CONS_TypeUDT *udt = cons__type_udt_from_record_type(root, record_type);
  if (udt != 0){
    CONS_TypeMember *member = push_array(root->arena, CONS_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    
    root->total_member_count += 1;
    
    member->kind = RADDBG_MemberKind_StaticData;
    member->name = push_str8_copy(root->arena, name);
    member->type = mem_type;
  }
}

static void
cons_type_add_member_method(CONS_Root *root, CONS_Type *record_type,
                            String8 name, CONS_Type *mem_type){
  CONS_TypeUDT *udt = cons__type_udt_from_record_type(root, record_type);
  if (udt != 0){
    CONS_TypeMember *member = push_array(root->arena, CONS_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    
    root->total_member_count += 1;
    
    member->kind = RADDBG_MemberKind_Method;
    member->name = push_str8_copy(root->arena, name);
    member->type = mem_type;
  }
}

static void
cons_type_add_member_static_method(CONS_Root *root, CONS_Type *record_type,
                                   String8 name, CONS_Type *mem_type){
  CONS_TypeUDT *udt = cons__type_udt_from_record_type(root, record_type);
  if (udt != 0){
    CONS_TypeMember *member = push_array(root->arena, CONS_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    
    root->total_member_count += 1;
    
    member->kind = RADDBG_MemberKind_StaticMethod;
    member->name = push_str8_copy(root->arena, name);
    member->type = mem_type;
  }
}

static void
cons_type_add_member_virtual_method(CONS_Root *root, CONS_Type *record_type,
                                    String8 name, CONS_Type *mem_type){
  CONS_TypeUDT *udt = cons__type_udt_from_record_type(root, record_type);
  if (udt != 0){
    CONS_TypeMember *member = push_array(root->arena, CONS_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    
    root->total_member_count += 1;
    
    member->kind = RADDBG_MemberKind_VirtualMethod;
    member->name = push_str8_copy(root->arena, name);
    member->type = mem_type;
  }
}

static void
cons_type_add_member_base(CONS_Root *root, CONS_Type *record_type,
                          CONS_Type *base_type, U32 off){
  CONS_TypeUDT *udt = cons__type_udt_from_record_type(root, record_type);
  if (udt != 0){
    CONS_TypeMember *member = push_array(root->arena, CONS_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    
    root->total_member_count += 1;
    
    member->kind = RADDBG_MemberKind_Base;
    member->type = base_type;
    member->off = off;
  }
}

static void
cons_type_add_member_virtual_base(CONS_Root *root, CONS_Type *record_type,
                                  CONS_Type *base_type, U32 vptr_off, U32 vtable_off){
  CONS_TypeUDT *udt = cons__type_udt_from_record_type(root, record_type);
  if (udt != 0){
    CONS_TypeMember *member = push_array(root->arena, CONS_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    
    root->total_member_count += 1;
    
    member->kind = RADDBG_MemberKind_VirtualBase;
    member->type = base_type;
    // TODO(allen): what to do with the two offsets in this case?
  }
}

static void
cons_type_add_member_nested_type(CONS_Root *root, CONS_Type *record_type,
                                 CONS_Type *nested_type){
  CONS_TypeUDT *udt = cons__type_udt_from_record_type(root, record_type);
  if (udt != 0){
    CONS_TypeMember *member = push_array(root->arena, CONS_TypeMember, 1);
    SLLQueuePush(udt->first_member, udt->last_member, member);
    udt->member_count += 1;
    
    root->total_member_count += 1;
    
    member->kind = RADDBG_MemberKind_NestedType;
    member->type = nested_type;
  }
}

static void
cons_type_add_enum_val(CONS_Root *root, CONS_Type *enum_type, String8 name, U64 val){
  if (enum_type->kind != RADDBG_TypeKind_Enum){
    // TODO(allen): API error
  }
  else{
    CONS_TypeUDT *udt = cons__type_udt_from_any_type(root, enum_type);
    
    CONS_TypeEnumVal *enum_val = push_array(root->arena, CONS_TypeEnumVal, 1);
    SLLQueuePush(udt->first_enum_val, udt->last_enum_val, enum_val);
    udt->enum_val_count += 1;
    
    root->total_enum_val_count += 1;
    
    enum_val->name = push_str8_copy(root->arena, name);
    enum_val->val  = val;
  }
}

static void
cons_type_set_source_coordinates(CONS_Root *root, CONS_Type *defined_type,
                                 String8 source_path, U32 line, U32 col){
  if (!(RADDBG_TypeKind_FirstUserDefined <= defined_type->kind &&
        defined_type->kind <= RADDBG_TypeKind_LastUserDefined)){
    // TODO(allen): API error
  }
  else{
    CONS_TypeUDT *udt = cons__type_udt_from_any_type(root, defined_type);
    
    udt->source_path = push_str8_copy(root->arena, source_path);
    udt->line = line;
    udt->col = col;
  }
}

// type list

static void
cons_type_list_push(Arena *arena, CONS_TypeList *list, CONS_Type *type){
  CONS_TypeNode *node = push_array(arena, CONS_TypeNode, 1);
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
  node->type = type;
}

// symbols

static CONS_Symbol*
cons_symbol_handle_from_user_id(CONS_Root *root, U64 symbol_user_id){
  CONS__U64ToPtrLookup lookup = {0};
  cons__u64toptr_lookup(&root->symbol_map, symbol_user_id, &lookup);
  
  CONS_Symbol *result = 0;
  if (lookup.match != 0){
    result = (CONS_Symbol*)lookup.match;
  }
  else{
    result = push_array(root->arena, CONS_Symbol, 1);
    SLLQueuePush_N(root->first_symbol, root->last_symbol, result, next_order);
    root->symbol_count += 1;
    cons__u64toptr_insert(root->arena, &root->symbol_map, symbol_user_id, &lookup, result);
  }
  
  return(result);
}

static void
cons_symbol_set_info(CONS_Root *root, CONS_Symbol *symbol, CONS_SymbolInfo *info){
  CONS_SymbolKind kind = info->kind;
  
  if (symbol->kind != CONS_SymbolKind_NULL){
    // TODO(allen): API error
  }
  else if (kind == CONS_SymbolKind_NULL || kind >= CONS_SymbolKind_COUNT){
    // TODO(allen): API error
  }
  else if (info->type == 0){
    // TODO(allen): API error
  }
  else{
    CONS_Symbol *container_symbol = info->container_symbol;
    CONS_Type *container_type = info->container_type;
    if (info->container_symbol != 0 && info->container_type != 0){
      // TODO(allen): API error
      container_type = 0;
    }
    
    root->symbol_kind_counts[kind] += 1;
    symbol->idx = root->symbol_kind_counts[kind];
    
    symbol->kind = kind;
    symbol->name = push_str8_copy(root->arena, info->name);
    symbol->link_name = push_str8_copy(root->arena, info->link_name);
    symbol->type = info->type;
    symbol->is_extern = info->is_extern;
    symbol->offset = info->offset;
    symbol->container_symbol = container_symbol;
    symbol->container_type = container_type;
    
    // set root scope
    switch (kind){
      default:{}break;
      case CONS_SymbolKind_GlobalVariable:
      case CONS_SymbolKind_ThreadVariable:
      {
        if (info->root_scope != 0){
          // TODO(allen): API error
        }
      }break;
      
      case CONS_SymbolKind_Procedure:
      {
        if (info->root_scope == 0){
          // TODO(allen): API error
        }
        else{
          symbol->root_scope = info->root_scope;
          cons__scope_recursive_set_symbol(info->root_scope, symbol);
        }
      }break;
    }
    
    // save name map
    {
      CONS__NameMap *map = 0;
      switch (kind){
        default:{}break;
        case CONS_SymbolKind_GlobalVariable:
        {
          map = cons__name_map_for_kind(root, RADDBG_NameMapKind_GlobalVariables);
        }break;
        case CONS_SymbolKind_ThreadVariable:
        {
          map = cons__name_map_for_kind(root, RADDBG_NameMapKind_ThreadVariables);
        }break;
        case CONS_SymbolKind_Procedure:
        {
          map = cons__name_map_for_kind(root, RADDBG_NameMapKind_Procedures);
        }break;
      }
      if (map != 0){
        cons__name_map_add_pair(root, map, symbol->name, symbol->idx);
      }
    }
    
    // save link name map
    if (kind == CONS_SymbolKind_Procedure && symbol->link_name.size > 0){
      CONS__NameMap *map = cons__name_map_for_kind(root, RADDBG_NameMapKind_LinkNameProcedures);
      cons__name_map_add_pair(root, map, symbol->link_name, symbol->idx);
    }
  }
}

// scopes

static CONS_Scope*
cons_scope_handle_from_user_id(CONS_Root *root, U64 scope_user_id){
  CONS__U64ToPtrLookup lookup = {0};
  cons__u64toptr_lookup(&root->scope_map, scope_user_id, &lookup);
  
  CONS_Scope *result = 0;
  if (lookup.match != 0){
    result = (CONS_Scope*)lookup.match;
  }
  else{
    result = push_array(root->arena, CONS_Scope, 1);
    result->idx = root->scope_count;
    SLLQueuePush_N(root->first_scope, root->last_scope, result, next_order);
    root->scope_count += 1;
    cons__u64toptr_insert(root->arena, &root->scope_map, scope_user_id, &lookup, result);
  }
  
  return(result);
}

static void
cons_scope_set_parent(CONS_Root *root, CONS_Scope *scope, CONS_Scope *parent){
  if (scope->parent_scope != 0){
    // TODO(allen): API error
  }
  else if (parent == 0){
    // TODO(allen): API error
  }
  else{
    scope->symbol = parent->symbol;
    scope->parent_scope = parent;
    SLLQueuePush_N(parent->first_child, parent->last_child, scope, next_sibling);
  }
}

static void
cons_scope_add_voff_range(CONS_Root *root, CONS_Scope *scope, U64 voff_first, U64 voff_opl){
  CONS__VOffRange *range = push_array(root->arena, CONS__VOffRange, 1);
  SLLQueuePush(scope->first_range, scope->last_range, range);
  scope->range_count += 1;
  range->voff_first = voff_first;
  range->voff_opl   = voff_opl;
  scope->voff_base  = Min(scope->voff_base, voff_first);
  root->scope_voff_count += 2;
}

// locals

static CONS_Local*
cons_local_handle_from_user_id(CONS_Root *root, U64 local_user_id){
  CONS__U64ToPtrLookup lookup = {0};
  cons__u64toptr_lookup(&root->local_map, local_user_id, &lookup);
  
  CONS_Local *result = 0;
  if (lookup.match != 0){
    result = (CONS_Local*)lookup.match;
  }
  else{
    result = push_array(root->arena, CONS_Local, 1);
    cons__u64toptr_insert(root->arena, &root->local_map, local_user_id, &lookup, result);
  }
  
  return(result);
}

static void
cons_local_set_basic_info(CONS_Root *root, CONS_Local *local, CONS_LocalInfo *info){
  if (local->kind != RADDBG_LocalKind_NULL){
    // TODO(allen): API error
  }
  else if (info->scope == 0){
    // TODO(allen): API error
  }
  else if (info->kind == RADDBG_LocalKind_NULL || RADDBG_LocalKind_COUNT <= info->kind){
    // TODO(allen): API error
  }
  else if (info->type == 0){
    // TODO(allen): API error
  }
  else{
    CONS_Scope *scope = info->scope;
    SLLQueuePush(scope->first_local, scope->last_local, local);
    scope->local_count += 1;
    root->local_count += 1;
    local->kind = info->kind;
    local->name = push_str8_copy(root->arena, info->name);
    local->type = info->type;
  }
}

static CONS_LocationSet*
cons_location_set_from_local(CONS_Root *root, CONS_Local *local){
  CONS_LocationSet *result = local->locset;
  if (result == 0){
    local->locset = push_array(root->arena, CONS_LocationSet, 1);
    result = local->locset;
  }
  return(result);
}

static void
cons_location_set_add_case(CONS_Root *root, CONS_LocationSet *locset,
                           U64 voff_first, U64 voff_opl, CONS_Location *location){
  CONS__LocationCase *location_case = push_array(root->arena, CONS__LocationCase, 1);
  SLLQueuePush(locset->first_location_case, locset->last_location_case, location_case);
  locset->location_case_count += 1;
  root->location_count += 1;
  
  location_case->voff_first = voff_first;
  location_case->voff_opl   = voff_opl;
  location_case->location   = location;
}

static CONS_Location*
cons_location_addr_bytecode_stream(CONS_Root *root, CONS_EvalBytecode *bytecode){
  CONS_Location *result = push_array(root->arena, CONS_Location, 1);
  result->kind = RADDBG_LocationKind_AddrBytecodeStream;
  result->bytecode = *bytecode;
  return(result);
}

static CONS_Location*
cons_location_val_bytecode_stream(CONS_Root *root, CONS_EvalBytecode *bytecode){
  CONS_Location *result = push_array(root->arena, CONS_Location, 1);
  result->kind = RADDBG_LocationKind_ValBytecodeStream;
  result->bytecode = *bytecode;
  return(result);
}

static CONS_Location*
cons_location_addr_reg_plus_u16(CONS_Root *root, U8 reg_code, U16 offset){
  CONS_Location *result = push_array(root->arena, CONS_Location, 1);
  result->kind = RADDBG_LocationKind_AddrRegisterPlusU16;
  result->register_code = reg_code;
  result->offset = offset;
  return(result);
}

static CONS_Location*
cons_location_addr_addr_reg_plus_u16(CONS_Root *root, U8 reg_code, U16 offset){
  CONS_Location *result = push_array(root->arena, CONS_Location, 1);
  result->kind = RADDBG_LocationKind_AddrAddrRegisterPlusU16;
  result->register_code = reg_code;
  result->offset = offset;
  return(result);
}

static CONS_Location*
cons_location_val_reg(CONS_Root *root, U8 reg_code){
  CONS_Location *result = push_array(root->arena, CONS_Location, 1);
  result->kind = RADDBG_LocationKind_ValRegister;
  result->register_code = reg_code;
  return(result);
}

// bytecode

static void
cons_bytecode_push_op(Arena *arena, CONS_EvalBytecode *bytecode, RADDBG_EvalOp op, U64 p){
  U8 ctrlbits = raddbg_eval_opcode_ctrlbits[op];
  U32 p_size = RADDBG_DECODEN_FROM_CTRLBITS(ctrlbits);
  
  CONS_EvalBytecodeOp *node = push_array(arena, CONS_EvalBytecodeOp, 1);
  node->op = op;
  node->p_size = p_size;
  node->p = p;
  
  SLLQueuePush(bytecode->first_op, bytecode->last_op, node);
  bytecode->op_count += 1;
  bytecode->encoded_size += 1 + p_size;
}

static void
cons_bytecode_push_uconst(Arena *arena, CONS_EvalBytecode *bytecode, U64 x){
  if (x <= 0xFF){
    cons_bytecode_push_op(arena, bytecode, RADDBG_EvalOp_ConstU8, x);
  }
  else if (x <= 0xFFFF){
    cons_bytecode_push_op(arena, bytecode, RADDBG_EvalOp_ConstU16, x);
  }
  else if (x <= 0xFFFFFFFF){
    cons_bytecode_push_op(arena, bytecode, RADDBG_EvalOp_ConstU32, x);
  }
  else{
    cons_bytecode_push_op(arena, bytecode, RADDBG_EvalOp_ConstU64, x);
  }
}

static void
cons_bytecode_push_sconst(Arena *arena, CONS_EvalBytecode *bytecode, S64 x){
  if (-0x80 <= x && x <= 0x7F){
    cons_bytecode_push_op(arena, bytecode, RADDBG_EvalOp_ConstU8, (U64)x);
    cons_bytecode_push_op(arena, bytecode, RADDBG_EvalOp_TruncSigned, 8);
  }
  else if (-0x8000 <= x && x <= 0x7FFF){
    cons_bytecode_push_op(arena, bytecode, RADDBG_EvalOp_ConstU16, (U64)x);
    cons_bytecode_push_op(arena, bytecode, RADDBG_EvalOp_TruncSigned, 16);
  }
  else if (-0x80000000ll <= x && x <= 0x7FFFFFFFll){
    cons_bytecode_push_op(arena, bytecode, RADDBG_EvalOp_ConstU32, (U64)x);
    cons_bytecode_push_op(arena, bytecode, RADDBG_EvalOp_TruncSigned, 32);
  }
  else{
    cons_bytecode_push_op(arena, bytecode, RADDBG_EvalOp_ConstU64, (U64)x);
  }
}

static void
cons_bytecode_concat_in_place(CONS_EvalBytecode *left_dst, CONS_EvalBytecode *right_destroyed){
  if (right_destroyed->first_op != 0){
    if (left_dst->first_op == 0){
      MemoryCopyStruct(left_dst, right_destroyed);
    }
    else{
      left_dst->last_op = right_destroyed->last_op;
      left_dst->op_count += right_destroyed->op_count;
      left_dst->encoded_size += right_destroyed->encoded_size;
    }
    MemoryZeroStruct(right_destroyed);
  }
}



////////////////////////////////
//- Implementation Helpers

// types

static CONS_Type*
cons__type_new(CONS_Root *root){
  ProfBeginFunction();
  CONS_Type *result = push_array(root->arena, CONS_Type, 1);
  result->idx = root->type_count;
  SLLQueuePush_N(root->first_type, root->last_type, result, next_order);
  root->type_count += 1;
  ProfEnd();
  return(result);
}

static CONS_TypeUDT*
cons__type_udt_from_any_type(CONS_Root *root, CONS_Type *type){
  if (type->udt == 0){
    CONS_TypeUDT *new_udt = push_array(root->arena, CONS_TypeUDT, 1);
    new_udt->idx = root->type_udt_count;
    SLLQueuePush_N(root->first_udt, root->last_udt, new_udt, next_order);
    root->type_udt_count += 1;
    new_udt->self_type = type;
    type->udt = new_udt;
  }
  CONS_TypeUDT *result = type->udt;
  return(result);
}

static CONS_TypeUDT*
cons__type_udt_from_record_type(CONS_Root *root, CONS_Type *type){
  CONS_TypeUDT *result = 0;
  
  if (!(type->kind == RADDBG_TypeKind_Struct ||
        type->kind == RADDBG_TypeKind_Class ||
        type->kind == RADDBG_TypeKind_Union)){
    // TODO(allen): API error
  }
  else{
    result = cons__type_udt_from_any_type(root, type);
  }
  
  return(result);
}

// scopes

static void
cons__scope_recursive_set_symbol(CONS_Scope *scope, CONS_Symbol *symbol){
  scope->symbol = symbol;
  for (CONS_Scope *node = scope->first_child;
       node != 0;
       node = node->next_sibling){
    cons__scope_recursive_set_symbol(node, symbol);
  }
}

// name maps

static CONS__NameMap*
cons__name_map_for_kind(CONS_Root *root, RADDBG_NameMapKind kind){
  CONS__NameMap *result = 0;
  if (kind < RADDBG_NameMapKind_COUNT){
    if (root->name_maps[kind] == 0){
      root->name_maps[kind] = push_array(root->arena, CONS__NameMap, 1);
    }
    result = root->name_maps[kind];
  }
  return(result);
}

static void
cons__name_map_add_pair(CONS_Root *root, CONS__NameMap *map, String8 string, U32 idx){
  // hash
  U64 hash = raddbg_hash(string.str, string.size);
  U64 bucket_idx = hash%ArrayCount(map->buckets);
  
  // find existing name node
  CONS__NameMapNode *match = 0;
  for (CONS__NameMapNode *node = map->buckets[bucket_idx];
       node != 0;
       node = node->bucket_next){
    if (str8_match(string, node->string, 0)){
      match = node;
      break;
    }
  }
  
  // make name node if necessary
  if (match == 0){
    match = push_array(root->arena, CONS__NameMapNode, 1);
    match->string = push_str8_copy(root->arena, string);
    SLLStackPush_N(map->buckets[bucket_idx], match, bucket_next);
    SLLQueuePush_N(map->first, map->last, match, order_next);
    map->name_count += 1;
  }
  
  // find existing idx
  B32 existing_idx = 0;
  for (CONS__NameMapIdxNode *node = match->idx_first;
       node != 0;
       node = node->next){
    for (U32 i = 0; i < ArrayCount(node->idx); i += 1){
      if (node->idx[i] == 0){
        break;
      }
      if (node->idx[i] == idx){
        existing_idx = 1;
        break;
      }
    }
  }
  
  // insert new idx if necessary
  if (!existing_idx){
    CONS__NameMapIdxNode *idx_node = match->idx_last;
    
    U32 insert_i = match->idx_count%ArrayCount(idx_node->idx);
    if (insert_i == 0){
      idx_node = push_array(root->arena, CONS__NameMapIdxNode, 1);
      SLLQueuePush(match->idx_first, match->idx_last, idx_node);
    }
    
    idx_node->idx[insert_i] = idx;
    match->idx_count += 1;
  }
}

// u64 to ptr map

static void
cons__u64toptr_init(Arena *arena, CONS__U64ToPtrMap *map, U64 bucket_count){
  Assert(IsPow2OrZero(bucket_count) && bucket_count > 0);
  map->buckets = push_array(arena, CONS__U64ToPtrNode*, bucket_count);
  map->bucket_count = bucket_count;
}

static void
cons__u64toptr_lookup(CONS__U64ToPtrMap *map, U64 key, CONS__U64ToPtrLookup *lookup_out){
  ProfBeginFunction();
  U64 bucket_idx = key&(map->bucket_count - 1);
  CONS__U64ToPtrNode *check_node = map->buckets[bucket_idx];
  for (;check_node != 0; check_node = check_node->next){
    for (U32 k = 0; k < ArrayCount(check_node->key); k += 1){
      if (check_node->ptr[k] == 0){
        lookup_out->fill_node = check_node;
        lookup_out->fill_k = k;
        break;
      }
      else if (check_node->key[k] == key){
        lookup_out->match = check_node->ptr[k];
        break;
      }
    }
  }
  ProfEnd();
}

static void
cons__u64toptr_insert(Arena *arena, CONS__U64ToPtrMap *map, U64 key,
                      CONS__U64ToPtrLookup *lookup, void *ptr){
  if (lookup->fill_node != 0){
    CONS__U64ToPtrNode *node = lookup->fill_node;
    U32 k = lookup->fill_k;
    node->key[k] = key;
    node->ptr[k] = ptr;
  }
  else{
    U64 bucket_idx = key&(map->bucket_count - 1);
    
    CONS__U64ToPtrNode *node = push_array(arena, CONS__U64ToPtrNode, 1);
    SLLStackPush(map->buckets[bucket_idx], node);
    node->key[0] = key;
    node->ptr[0] = ptr;
    
    lookup->fill_node = node;
    lookup->fill_k = 0;
  }
}

// str8 to ptr map

static void*
cons__str8toptr_lookup(CONS__Str8ToPtrMap *map, String8 key, U64 hash){
  ProfBeginFunction();
  void *result = 0;
  U64 bucket_idx = hash%ArrayCount(map->buckets);
  for (CONS__Str8ToPtrNode *node = map->buckets[bucket_idx];
       node != 0;
       node = node->next){
    if (node->hash == hash && str8_match(node->key, key, 0)){
      result = node->ptr;
      break;
    }
  }
  ProfEnd();
  return(result);
}

static void
cons__str8toptr_insert(Arena *arena, CONS__Str8ToPtrMap *map, String8 key, U64 hash, void *ptr){
  ProfBeginFunction();
  U64 bucket_idx = hash%ArrayCount(map->buckets);
  
  CONS__Str8ToPtrNode *node = push_array(arena, CONS__Str8ToPtrNode, 1);
  SLLStackPush(map->buckets[bucket_idx], node);
  
  node->key  = push_str8_copy(arena, key);
  node->hash = hash;
  node->ptr = ptr;
  ProfEnd();
}


//- cons intermediate functions

static U32
cons__dsection(Arena *arena, CONS__DSections *dss, void *data, U64 size, RADDBG_DataSectionTag tag){
  U32 result = dss->count;
  
  CONS__DSectionNode *node = push_array(arena, CONS__DSectionNode, 1);
  SLLQueuePush(dss->first, dss->last, node);
  node->data = data;
  node->size = size;
  node->tag = tag;
  dss->count += 1;
  
  return(result);
}

static CONS__BakeCtx*
cons__bake_ctx_begin(void){
  Arena *arena = arena_alloc();
  CONS__BakeCtx *result = push_array(arena, CONS__BakeCtx, 1);
  result->arena = arena;
  
  cons__string(result, str8_lit(""));
  
  cons__idx_run(result, 0, 0);
  
  result->tree = push_array(arena, CONS__PathTree, 1);
  {
    CONS__PathNode *nil_path_node = cons__paths_new_node(result);
    nil_path_node->name = str8_lit("<NIL>");
    CONS__SrcNode *nil_src_node = cons__paths_new_src_node(result);
    nil_src_node->path_node = nil_path_node;
    nil_src_node->normal_full_path = str8_lit("<NIL>");
    nil_path_node->src_file = nil_src_node;
  }
  
  return(result);
}

static void
cons__bake_ctx_release(CONS__BakeCtx *bake_ctx){
  arena_release(bake_ctx->arena);
}


static U32
cons__string(CONS__BakeCtx *bctx, String8 str){
  Arena *arena = bctx->arena;
  CONS__Strings *strs = &bctx->strs;
  
  U64 hash = raddbg_hash(str.str, str.size);
  U64 bucket_idx = hash%ArrayCount(strs->buckets);
  
  // look for a match
  CONS__StringNode *match = 0;
  for (CONS__StringNode *node = strs->buckets[bucket_idx];
       node != 0;
       node = node->bucket_next){
    if (node->hash == hash &&
        str8_match(node->str, str, 0)){
      match = node;
      break;
    }
  }
  
  // insert new node if no match
  if (match == 0){
    CONS__StringNode *node = push_array_no_zero(arena, CONS__StringNode, 1);
    node->str = push_str8_copy(arena, str);
    node->hash = hash;
    node->idx = strs->count;
    strs->count += 1;
    
    SLLQueuePush_N(strs->order_first, strs->order_last, node, order_next);
    SLLStackPush_N(strs->buckets[bucket_idx], node, bucket_next);
    
    match = node;
  }
  
  // extract idx to return
  Assert(match != 0);
  U32 result = match->idx;
  
  return(result);
}

static U64
cons__idx_run_hash(U32 *idx_run, U32 count){
  U64 hash = 5381;
  U32 *ptr = idx_run;
  U32 *opl = idx_run + count;
  for (; ptr < opl; ptr += 1){
    hash = ((hash << 5) + hash) + (*ptr);
  }
  return(hash);
}

static U32
cons__idx_run(CONS__BakeCtx *bctx, U32 *idx_run, U32 count){
  Arena *arena = bctx->arena;
  CONS__IdxRuns *idxs = &bctx->idxs;
  
  U64 hash = cons__idx_run_hash(idx_run, count);
  U64 bucket_idx = hash%ArrayCount(idxs->buckets);
  
  // look for a match
  CONS__IdxRunNode *match = 0;
  for (CONS__IdxRunNode *node = idxs->buckets[bucket_idx];
       node != 0;
       node = node->bucket_next){
    if (node->hash == hash){
      S32 is_match = 1;
      U32 *node_idx = node->idx_run;
      for (U32 i = 0; i < count; i += 1){
        if (node_idx[i] != idx_run[i]){
          is_match = 0;
          break;
        }
      }
      if (is_match){
        match = node;
        break;
      }
    }
  }
  
  // insert new node if no match
  if (match == 0){
    CONS__IdxRunNode *node = push_array_no_zero(arena, CONS__IdxRunNode, 1);
    U32 *idx_run_copy = push_array_no_zero(arena, U32, count);
    for (U32 i = 0; i < count; i += 1){
      idx_run_copy[i] = idx_run[i];
    }
    node->idx_run = idx_run_copy;
    node->hash = hash;
    node->count = count;
    node->first_idx = idxs->idx_count;
    
    idxs->count += 1;
    idxs->idx_count += count;
    
    SLLQueuePush_N(idxs->order_first, idxs->order_last, node, order_next);
    SLLStackPush_N(idxs->buckets[bucket_idx], node, bucket_next);
    
    match = node;
  }
  
  // extract idx to return
  Assert(match != 0);
  U32 result = match->first_idx;
  
  return(result);
}

static CONS__PathNode*
cons__paths_new_node(CONS__BakeCtx *bctx){
  CONS__PathTree *tree = bctx->tree;
  CONS__PathNode *result = push_array(bctx->arena, CONS__PathNode, 1);
  SLLQueuePush_N(tree->first, tree->last, result, next_order);
  result->idx = tree->count;
  tree->count += 1;
  return(result);
}

static CONS__PathNode*
cons__paths_sub_path(CONS__BakeCtx *bctx, CONS__PathNode *dir, String8 sub_dir){
  // look for existing match
  CONS__PathNode *match = 0;
  for (CONS__PathNode *node = dir->first_child;
       node != 0;
       node = node->next_sibling){
    if (str8_match(node->name, sub_dir, StringMatchFlag_CaseInsensitive)){
      match = node;
      break;
    }
  }
  
  // construct new node if no match
  CONS__PathNode *new_node = 0;
  if (match == 0){
    new_node = cons__paths_new_node(bctx);
    new_node->parent = dir;
    SLLQueuePush_N(dir->first_child, dir->last_child, new_node, next_sibling);
    new_node->name = push_str8_copy(bctx->arena, sub_dir);
  }
  
  // select result from the two paths
  CONS__PathNode *result = match;
  if (match == 0){
    result = new_node;
  }
  
  return(result);
}

static CONS__PathNode*
cons__paths_node_from_path(CONS__BakeCtx *bctx, String8 path){
  CONS__PathNode *node_cursor = &bctx->tree->root;
  
  U8 *ptr = path.str;
  U8 *opl = path.str + path.size;
  for (;ptr < opl;){
    // skip past slashes
    for (;ptr < opl && (*ptr == '/' || *ptr == '\\'); ptr += 1);
    
    // save beginning of non-slash range
    U8 *range_first = ptr;
    
    // skip past non-slashes
    for (;ptr < opl && !(*ptr == '/' || *ptr == '\\'); ptr += 1);
    
    // if range is non-empty advance the node cursor
    if (range_first < ptr){
      String8 sub_dir = str8_range(range_first, ptr);
      node_cursor = cons__paths_sub_path(bctx, node_cursor, sub_dir);
    }
  }
  
  CONS__PathNode *result = node_cursor;
  return(result);
}

static U32
cons__paths_idx_from_path(CONS__BakeCtx *bctx, String8 path){
  CONS__PathNode *node = cons__paths_node_from_path(bctx, path);
  U32 result = node->idx;
  return(result);
}

static CONS__SrcNode*
cons__paths_new_src_node(CONS__BakeCtx *bctx){
  CONS__PathTree *tree = bctx->tree;
  CONS__SrcNode *result = push_array(bctx->arena, CONS__SrcNode, 1);
  SLLQueuePush(tree->src_first, tree->src_last, result);
  result->idx = tree->src_count;
  tree->src_count += 1;
  return(result);
}

static CONS__SrcNode*
cons__paths_src_node_from_path_node(CONS__BakeCtx *bctx, CONS__PathNode *path_node){
  CONS__SrcNode *result = path_node->src_file;
  if (result == 0){
    CONS__SrcNode *new_node = cons__paths_new_src_node(bctx);
    new_node->path_node = path_node;
    new_node->normal_full_path = cons__normal_string_from_path_node(bctx->arena, path_node);
    result = path_node->src_file = new_node;
  }
  return(result);
}


//- cons path helper

static String8
cons__normal_string_from_path_node(Arena *arena, CONS__PathNode *node){
  Temp scratch = scratch_begin(&arena, 1);
  String8List list = {0};
  if (node != 0){
    cons__normal_string_from_path_node_build(scratch.arena, node, &list);
  }
  StringJoin join = {0};
  join.sep = str8_lit("/");
  String8 result = str8_list_join(arena, &list, &join);
  {
    U8 *ptr = result.str;
    U8 *opl = result.str + result.size;
    for (; ptr < opl; ptr += 1){
      U8 c = *ptr;
      if ('A' <= c && c <= 'Z') c += 'a' - 'A';
      *ptr = c;
    }
  }
  scratch_end(scratch);
  return(result);
}

static void
cons__normal_string_from_path_node_build(Arena *arena, CONS__PathNode *node, String8List *out){
  if (node->parent != 0){
    cons__normal_string_from_path_node_build(arena, node->parent, out);
  }
  if (node->name.size > 0){
    str8_list_push(arena, out, node->name);
  }
}


//- cons sort helper

static CONS__SortKey*
cons__sort_key_array(Arena *arena, CONS__SortKey *keys, U64 count){
  // This sort is designed to take advantage of lots of pre-existing sorted ranges.
  // Most line info is already sorted or close to already sorted.
  // Similarly most vmap data has lots of pre-sorted ranges. etc. etc.
  // Also - this sort should be a "stable" sort. In the use case of sorting vmap
  // ranges, we want to be able to rely on order, so it needs to be preserved here.
  
  ProfBegin("cons__sort_key_array");
  Temp scratch = scratch_begin(&arena, 1);
  
  CONS__SortKey *result = 0;
  
  if (count <= 1){
    result = keys;
  }
  else{
    CONS__OrderedRange *ranges_first = 0;
    CONS__OrderedRange *ranges_last = 0;
    U64 range_count = 0;
    {
      U64 pos = 0;
      for (;pos < count;){
        // identify ordered range
        U64 first = pos;
        U64 opl = pos + 1;
        for (; opl < count && keys[opl - 1].key <= keys[opl].key; opl += 1);
        
        // generate an ordered range node
        CONS__OrderedRange *new_range = push_array(scratch.arena, CONS__OrderedRange, 1);
        SLLQueuePush(ranges_first, ranges_last, new_range);
        range_count += 1;
        new_range->first = first;
        new_range->opl = opl;
        
        // update pos
        pos = opl;
      }
    }
    
    if (range_count == 1){
      result = keys;
    }
    else{
      CONS__SortKey *keys_swap = push_array_no_zero(arena, CONS__SortKey, count);
      
      CONS__SortKey *src = keys;
      CONS__SortKey *dst = keys_swap;
      
      CONS__OrderedRange *src_ranges = ranges_first;
      CONS__OrderedRange *dst_ranges = 0;
      CONS__OrderedRange *dst_ranges_last = 0;
      
      for (;;){
        // begin a pass
        for (;;){
          // end pass when out of ranges
          if (src_ranges == 0){
            break;
          }
          
          // get first range
          CONS__OrderedRange *range1 = src_ranges;
          SLLStackPop(src_ranges);
          
          // if this range is the whole array, we are done
          if (range1->first == 0 && range1->opl == count){
            result = src;
            goto sort_done;
          }
          
          // if there is not a second range, save this range for next time and end this pass
          if (src_ranges == 0){
            U64 first = range1->first;
            MemoryCopy(dst + first, src + first, sizeof(*src)*(range1->opl - first));
            SLLQueuePush(dst_ranges, dst_ranges_last, range1);
            break;
          }
          
          // get second range
          CONS__OrderedRange *range2 = src_ranges;
          SLLStackPop(src_ranges);
          
          Assert(range1->opl == range2->first);
          
          // merge these ranges
          U64 jd = range1->first;
          U64 j1 = range1->first;
          U64 j1_opl = range1->opl;
          U64 j2 = range2->first;
          U64 j2_opl = range2->opl;
          for (;;){
            if (src[j1].key <= src[j2].key){
              MemoryCopy(dst + jd, src + j1, sizeof(*src));
              j1 += 1;
              jd += 1;
              if (j1 >= j1_opl){
                break;
              }
            }
            else{
              MemoryCopy(dst + jd, src + j2, sizeof(*src));
              j2 += 1;
              jd += 1;
              if (j2 >= j2_opl){
                break;
              }
            }
          }
          if (j1 < j1_opl){
            MemoryCopy(dst + jd, src + j1, sizeof(*src)*(j1_opl - j1));
          }
          else{
            MemoryCopy(dst + jd, src + j2, sizeof(*src)*(j2_opl - j2));
          }
          
          // save this as one range
          range1->opl = range2->opl;
          SLLQueuePush(dst_ranges, dst_ranges_last, range1);
        }
        
        // end pass by swapping buffers and range nodes
        Swap(CONS__SortKey*, src, dst);
        src_ranges = dst_ranges;
        dst_ranges = 0;
        dst_ranges_last = 0;
      }
    }
  }
  sort_done:;
  
#if 0
  // assert sortedness
  for (U64 i = 1; i < count; i += 1){
    Assert(result[i - 1].key <= result[i].key);
  }
#endif
  
  scratch_end(scratch);
  ProfEnd();
  
  return(result);
}


//- cons intermediate unit line info
static CONS__UnitLinesCombined*
cons__unit_combine_lines(Arena *arena, CONS__BakeCtx *bctx, CONS_LineSequenceNode *first_seq){
  ProfBegin("cons__unit_combine_lines");
  Temp scratch = scratch_begin(&arena, 1);
  
  // gather up all line info into two arrays
  //  keys: sortable array; pairs voffs with line info records; null records are sequence enders
  //  recs: contains all the source coordinates for a range of voffs
  U64 line_count = 0;
  U64 seq_count = 0;
  for (CONS_LineSequenceNode *node = first_seq;
       node != 0;
       node = node->next){
    seq_count += 1;
    line_count += node->line_seq.line_count;
  }
  
  U64 key_count = line_count + seq_count;
  CONS__SortKey *line_keys = push_array_no_zero(scratch.arena, CONS__SortKey, key_count);
  CONS__LineRec *line_recs = push_array_no_zero(scratch.arena, CONS__LineRec, line_count);
  
  {
    CONS__SortKey *key_ptr = line_keys;
    CONS__LineRec *rec_ptr = line_recs;
    
    for (CONS_LineSequenceNode *node = first_seq;
         node != 0;
         node = node->next){
      CONS__PathNode *src_path =
        cons__paths_node_from_path(bctx, node->line_seq.file_name);
      CONS__SrcNode *src_file  = cons__paths_src_node_from_path_node(bctx, src_path);
      U32 file_id = src_file->idx;
      
      U64 node_line_count = node->line_seq.line_count;
      for (U64 i = 0; i < node_line_count; i += 1){
        key_ptr->key = node->line_seq.voffs[i];
        key_ptr->val = rec_ptr;
        key_ptr += 1;
        
        rec_ptr->file_id = file_id;
        rec_ptr->line_num = node->line_seq.line_nums[i];
        if (node->line_seq.col_nums != 0){
          rec_ptr->col_first = node->line_seq.col_nums[i*2];
          rec_ptr->col_opl = node->line_seq.col_nums[i*2 + 1];
        }
        rec_ptr += 1;
      }
      
      key_ptr->key = node->line_seq.voffs[node_line_count];
      key_ptr->val = 0;
      key_ptr += 1;
      
      CONS__LineMapFragment *fragment = push_array(arena, CONS__LineMapFragment, 1);
      SLLQueuePush(src_file->first_fragment, src_file->last_fragment, fragment);
      fragment->sequence = node;
    }
  }
  
  // sort
  CONS__SortKey *sorted_line_keys = cons__sort_key_array(scratch.arena, line_keys, key_count);
  
  // TODO(allen): do a pass over sorted keys to make sure duplicate keys are sorted with
  // null record first, and no more than one null record and one non-null record
  
  // arrange output
  U64 *arranged_voffs = push_array_no_zero(arena, U64, key_count + 1);
  RADDBG_Line *arranged_lines = push_array_no_zero(arena, RADDBG_Line, key_count);
  
  for (U64 i = 0; i < key_count; i += 1){
    arranged_voffs[i] = sorted_line_keys[i].key;
  }
  arranged_voffs[key_count] = ~0ull;
  for (U64 i = 0; i < key_count; i += 1){
    CONS__LineRec *rec = (CONS__LineRec*)sorted_line_keys[i].val;
    if (rec != 0){
      arranged_lines[i].file_idx = rec->file_id;
      arranged_lines[i].line_num = rec->line_num;
    }
    else{
      arranged_lines[i].file_idx = 0;
      arranged_lines[i].line_num = 0;
    }
  }
  
  CONS__UnitLinesCombined *result = push_array(arena, CONS__UnitLinesCombined, 1);
  result->voffs = arranged_voffs;
  result->lines = arranged_lines;
  result->cols = 0;
  result->line_count = key_count;
  
  scratch_end(scratch);
  ProfEnd();
  
  return(result);
}


//- cons intermediate source line info
static CONS__SrcLinesCombined*
cons__source_combine_lines(Arena *arena, CONS__LineMapFragment *first){
  ProfBegin("cons__source_combine_lines");
  Temp scratch = scratch_begin(&arena, 1);
  
  // gather line number map
  CONS__SrcLineMapBucket *first_bucket = 0;
  CONS__SrcLineMapBucket *last_bucket = 0;
  
  U64 line_count = 0;
  U64 voff_count = 0;
  U64 max_line_num = 0;
  for (CONS__LineMapFragment *map_fragment = first;
       map_fragment != 0;
       map_fragment = map_fragment->next){
    CONS_LineSequence *sequence = &map_fragment->sequence->line_seq;
    
    U64 *seq_voffs = sequence->voffs;
    U32 *seq_line_nums = sequence->line_nums;
    U64 seq_line_count = sequence->line_count;
    for (U64 i = 0; i < seq_line_count; i += 1){
      U32 line_num = seq_line_nums[i];
      U64 voff = seq_voffs[i];
      
      // update unique voff counter & max line number
      voff_count += 1;
      max_line_num = Max(max_line_num, line_num);
      
      // find match
      CONS__SrcLineMapBucket *match = 0;
      for (CONS__SrcLineMapBucket *node = first_bucket;
           node != 0;
           node = node->next){
        if (node->line_num == line_num){
          match = node;
          break;
        }
      }
      
      // introduce new line if no match
      if (match == 0){
        match = push_array(scratch.arena, CONS__SrcLineMapBucket, 1);
        SLLQueuePush(first_bucket, last_bucket, match);
        match->line_num = line_num;
        line_count += 1;
      }
      
      // insert new voff
      {
        CONS__SrcLineMapVoffBlock *block = push_array(scratch.arena, CONS__SrcLineMapVoffBlock, 1);
        SLLQueuePush(match->first_voff_block, match->last_voff_block, block);
        match->voff_count += 1;
        block->voff = voff;
      }
    }
  }
  
  // bake sortable keys array
  CONS__SortKey *keys = push_array_no_zero(scratch.arena, CONS__SortKey, line_count);
  {
    CONS__SortKey *key_ptr = keys;
    for (CONS__SrcLineMapBucket *node = first_bucket;
         node != 0;
         node = node->next, key_ptr += 1){
      key_ptr->key = node->line_num;
      key_ptr->val = node;
    }
  }
  
  // sort
  CONS__SortKey *sorted_keys = cons__sort_key_array(scratch.arena, keys, line_count);
  
  // bake result
  U32 *line_nums = push_array_no_zero(arena, U32, line_count);
  U32 *line_ranges = push_array_no_zero(arena, U32, line_count + 1);
  U64 *voffs = push_array_no_zero(arena, U64, voff_count);
  
  {
    U64 *voff_ptr = voffs;
    for (U32 i = 0; i < line_count; i += 1){
      line_nums[i] = sorted_keys[i].key;
      line_ranges[i] = (U32)(voff_ptr - voffs);
      CONS__SrcLineMapBucket *bucket = (CONS__SrcLineMapBucket*)sorted_keys[i].val;
      for (CONS__SrcLineMapVoffBlock *node = bucket->first_voff_block;
           node != 0;
           node = node->next){
        *voff_ptr = node->voff;
        voff_ptr += 1;
      }
    }
    line_ranges[line_count] = voff_count;
  }
  
  CONS__SrcLinesCombined *result = push_array(arena, CONS__SrcLinesCombined, 1);
  result->line_nums = line_nums;
  result->line_ranges = line_ranges;
  result->line_count = line_count;
  result->voffs = voffs;
  result->voff_count = voff_count;
  
  scratch_end(scratch);
  ProfEnd();
  
  return(result);
}


//- cons intermediate vmap type
static CONS__VMap*
cons__vmap_from_markers(Arena *arena, CONS__VMapMarker *markers, CONS__SortKey *keys, U64 marker_count){
  Temp scratch = scratch_begin(&arena, 1);
  
  // sort markers
  CONS__SortKey *sorted_keys = cons__sort_key_array(scratch.arena, keys, marker_count);
  
  // determine if an extra vmap entry for zero is needed
  U32 extra_vmap_entry = 0;
  if (marker_count > 0 && sorted_keys[0].key != 0){
    extra_vmap_entry = 1;
  }
  
  // fill output vmap entries
  U32 vmap_count_raw = marker_count - 1 + extra_vmap_entry;
  RADDBG_VMapEntry *vmap = push_array_no_zero(arena, RADDBG_VMapEntry, vmap_count_raw + 1);
  U32 vmap_entry_count_pass_1 = 0;
  
  {
    RADDBG_VMapEntry *vmap_ptr = vmap;
    
    if (extra_vmap_entry){
      vmap_ptr->voff = 0;
      vmap_ptr->idx = 0;
      vmap_ptr += 1;
    }
    
    CONS__VMapRangeTracker *tracker_stack = 0;
    CONS__VMapRangeTracker *tracker_free = 0;
    
    CONS__SortKey *key_ptr = sorted_keys;
    CONS__SortKey *key_opl = sorted_keys + marker_count;
    for (;key_ptr < key_opl;){
      // get initial map state from tracker stack
      U32 initial_idx = max_U32;
      if (tracker_stack != 0){
        initial_idx = tracker_stack->idx;
      }
      
      // update tracker stack
      // * we must process _all_ of the changes that apply at this voff before moving on
      U64 voff = key_ptr->key;
      
      for (;key_ptr < key_opl && key_ptr->key == voff; key_ptr += 1){
        CONS__VMapMarker *marker = (CONS__VMapMarker*)key_ptr->val;
        U32 idx = marker->idx;
        
        // push to stack
        if (marker->begin_range){
          CONS__VMapRangeTracker *new_tracker = tracker_free;
          if (new_tracker != 0){
            SLLStackPop(tracker_free);
          }
          else{
            new_tracker = push_array(scratch.arena, CONS__VMapRangeTracker, 1);
          }
          SLLStackPush(tracker_stack, new_tracker);
          new_tracker->idx = idx;
        }
        
        // pop matching node from stack (not always the top)
        else{
          CONS__VMapRangeTracker **ptr_in = &tracker_stack;
          CONS__VMapRangeTracker *match = 0;
          for (CONS__VMapRangeTracker *node = tracker_stack;
               node != 0;){
            if (node->idx == idx){
              match = node;
              break;
            }
            ptr_in = &node->next;
            node = node->next;
          }
          if (match != 0){
            *ptr_in = match->next;
            SLLStackPush(tracker_free, match);
          }
        }
      }
      
      // get final map state from tracker stack
      U32 final_idx = 0;
      if (tracker_stack != 0){
        final_idx = tracker_stack->idx;
      }
      
      // if final is different from initial - emit new vmap entry
      if (final_idx != initial_idx){
        vmap_ptr->voff = voff;
        vmap_ptr->idx = final_idx;
        vmap_ptr += 1;
      }
    }
    
    vmap_entry_count_pass_1 = (U32)(vmap_ptr - vmap);
  }
  
  // replace zero unit indexes that follow a non-zero
  // TODO(rjf): 0 *is* a real unit index right now
  if(0)
  {
    //  (the last entry is not replaced because it acts as a terminator)
    U32 last = vmap_entry_count_pass_1 - 1;
    
    RADDBG_VMapEntry *vmap_ptr = vmap;
    U64 real_idx = 0;
    
    for (U32 i = 0; i < last; i += 1, vmap_ptr += 1){
      // is this a zero after a real index?
      if (vmap_ptr->idx == 0){
        vmap_ptr->idx = real_idx;
      }
      
      // remember a real index
      else{
        real_idx = vmap_ptr->idx;
      }
    }
  }
  
  // combine duplicate neighbors
  U32 vmap_entry_count = 0;
  {
    RADDBG_VMapEntry *vmap_ptr = vmap;
    RADDBG_VMapEntry *vmap_opl = vmap + vmap_entry_count_pass_1;
    RADDBG_VMapEntry *vmap_out = vmap;
    
    for (;vmap_ptr < vmap_opl;){
      RADDBG_VMapEntry *vmap_range_first = vmap_ptr;
      U64 idx = vmap_ptr->idx;
      vmap_ptr += 1;
      for (;vmap_ptr < vmap_opl && vmap_ptr->idx == idx;) vmap_ptr += 1;
      MemoryCopyStruct(vmap_out, vmap_range_first);
      vmap_out += 1;
    }
    
    vmap_entry_count = (U32)(vmap_out - vmap);
  }
  
  // fill result
  CONS__VMap *result = push_array(arena, CONS__VMap, 1);
  result->vmap = vmap;
  result->count = vmap_entry_count - 1;
  
  scratch_end(scratch);
  
  return(result);
}


//- cons intermediate unit vmap
static CONS__VMap*
cons__vmap_from_unit_ranges(Arena *arena, CONS_UnitVMapRange *first, U64 count){
  Temp scratch = scratch_begin(&arena, 1);
  
  // count necessary markers
  U64 marker_count = count*2;
  
  // fill markers
  CONS__SortKey    *keys = push_array_no_zero(scratch.arena, CONS__SortKey, marker_count);
  CONS__VMapMarker *markers = push_array_no_zero(scratch.arena, CONS__VMapMarker, marker_count);
  
  {
    CONS__SortKey *key_ptr = keys;
    CONS__VMapMarker *marker_ptr = markers;
    for (CONS_UnitVMapRange *range = first;
         range != 0;
         range = range->next){
      if (range->first < range->opl){
        U32 unit_idx = range->unit->idx;
        
        key_ptr->key = range->first;
        key_ptr->val = marker_ptr;
        marker_ptr->idx = unit_idx;
        marker_ptr->begin_range = 1;
        key_ptr += 1;
        marker_ptr += 1;
        
        key_ptr->key = range->opl;
        key_ptr->val = marker_ptr;
        marker_ptr->idx = unit_idx;
        marker_ptr->begin_range = 0;
        key_ptr += 1;
        marker_ptr += 1;
      }
    }
  }
  
  // construct vmap
  CONS__VMap *result = cons__vmap_from_markers(arena, markers, keys, marker_count);
  
  scratch_end(scratch);
  
  return(result);
}

//- cons intermediate types
static CONS__TypeData*
cons__type_data_combine(Arena *arena, CONS_Root *root, CONS__BakeCtx *bctx){
  ProfBegin("cons__type_data_combine");
  Temp scratch = scratch_begin(&arena, 1);
  
  // fill type nodes
  U32 type_count = root->type_count;
  RADDBG_TypeNode *type_nodes = push_array_no_zero(arena, RADDBG_TypeNode, type_count);
  
  {
    RADDBG_TypeNode *ptr = type_nodes;
    RADDBG_TypeNode *opl = ptr + type_count;
    CONS_Type *loose_type = root->first_type;
    for (;loose_type != 0 && ptr < opl;
         loose_type = loose_type->next_order, ptr += 1){
      
      RADDBG_TypeKind kind = loose_type->kind;
      
      // shared
      ptr->kind = kind;
      ptr->flags = loose_type->flags;
      ptr->byte_size = loose_type->byte_size;
      
      // built-in
      if (RADDBG_TypeKind_FirstBuiltIn <= kind && kind <= RADDBG_TypeKind_LastBuiltIn){
        ptr->built_in.name_string_idx = cons__string(bctx, loose_type->name);
      }
      
      // constructed
      else if (RADDBG_TypeKind_FirstConstructed <= kind && kind <= RADDBG_TypeKind_LastConstructed){
        ptr->constructed.direct_type_idx = loose_type->direct_type->idx;
        
        switch (kind){
          case RADDBG_TypeKind_Array:
          {
            ptr->constructed.count = loose_type->count;
          }break;
          
          case RADDBG_TypeKind_Function:
          {
            // parameters
            U32 count = loose_type->count;
            U32 *idx_run = cons__idx_run_from_types(scratch.arena, loose_type->param_types, count);
            ptr->constructed.param_idx_run_first = cons__idx_run(bctx, idx_run, count);
            ptr->constructed.count = count;
          }break;
          
          case RADDBG_TypeKind_Method:
          {
            // parameters
            U32 count = loose_type->count;
            U32 *idx_run = cons__idx_run_from_types(scratch.arena, loose_type->param_types, count);
            ptr->constructed.param_idx_run_first = cons__idx_run(bctx, idx_run, count);
            ptr->constructed.count = count;
          }break;
        }
      }
      
      // user-defined
      else if (RADDBG_TypeKind_FirstUserDefined <= kind && kind <= RADDBG_TypeKind_LastUserDefined){
        ptr->user_defined.name_string_idx = cons__string(bctx, loose_type->name);
        if (loose_type->udt != 0){
          ptr->user_defined.udt_idx = loose_type->udt->idx;
        }
        if (loose_type->direct_type != 0){
          ptr->user_defined.direct_type_idx = loose_type->direct_type->idx;
        }
      }
      
      // bitfield
      else if (kind == RADDBG_TypeKind_Bitfield){
        ptr->bitfield.off = loose_type->off;
        ptr->bitfield.size = loose_type->count;
      }
      
      temp_end(scratch);
    }
    
    // both iterators should end at the same time
    Assert(loose_type == 0);
    Assert(ptr == opl);
  }
  
  
  // fill udts
  U32 udt_count = root->type_udt_count;
  RADDBG_UDT *udts = push_array_no_zero(arena, RADDBG_UDT, udt_count);
  
  U32 member_count = root->total_member_count;
  RADDBG_Member *members = push_array_no_zero(arena, RADDBG_Member, member_count);
  
  U32 enum_member_count = root->total_enum_val_count;
  RADDBG_EnumMember *enum_members = push_array_no_zero(arena, RADDBG_EnumMember, enum_member_count);
  
  {
    RADDBG_UDT *ptr = udts;
    RADDBG_UDT *opl = ptr + udt_count;
    
    RADDBG_Member *member_ptr = members;
    RADDBG_Member *member_opl = members + member_count;
    
    RADDBG_EnumMember *enum_member_ptr = enum_members;
    RADDBG_EnumMember *enum_member_opl = enum_members + enum_member_count;
    
    CONS_TypeUDT *loose_udt = root->first_udt;
    for (;loose_udt != 0 && ptr < opl;
         loose_udt = loose_udt->next_order, ptr += 1){
      ptr->self_type_idx = loose_udt->self_type->idx;
      
      Assert(loose_udt->member_count == 0 ||
             loose_udt->enum_val_count == 0);
      
      // enum members
      if (loose_udt->enum_val_count != 0){
        ptr->flags |= RADDBG_UserDefinedTypeFlag_EnumMembers;
        
        ptr->member_first = (U32)(enum_member_ptr - enum_members);
        ptr->member_count = loose_udt->enum_val_count;
        
        U32 local_enum_val_count = loose_udt->enum_val_count;
        CONS_TypeEnumVal *loose_enum_val = loose_udt->first_enum_val;
        for (U32 i = 0;
             i < local_enum_val_count;
             i += 1, enum_member_ptr += 1, loose_enum_val = loose_enum_val->next){
          enum_member_ptr->name_string_idx = cons__string(bctx, loose_enum_val->name);
          enum_member_ptr->val = loose_enum_val->val;
        }
      }
      
      // struct/class/union members
      else{
        ptr->member_first = (U32)(member_ptr - members);
        ptr->member_count = loose_udt->member_count;
        
        U32 local_member_count = loose_udt->member_count;
        CONS_TypeMember *loose_member = loose_udt->first_member;
        for (U32 i = 0;
             i < local_member_count;
             i += 1, member_ptr += 1, loose_member = loose_member->next){
          member_ptr->kind = loose_member->kind;
          // TODO(allen): member_ptr->visibility = ;
          member_ptr->name_string_idx = cons__string(bctx, loose_member->name);
          member_ptr->off = loose_member->off;
          member_ptr->type_idx = loose_member->type->idx;
          
          // TODO(allen): 
          if (loose_member->kind == RADDBG_MemberKind_Method){
            //loose_member_ptr->unit_idx = ;
            //loose_member_ptr->proc_symbol_idx = ;
          }
        }
        
      }
      
      U32 file_idx = 0;
      if (loose_udt->source_path.size > 0){
        CONS__PathNode *path_node = cons__paths_node_from_path(bctx, loose_udt->source_path);
        CONS__SrcNode  *src_node  = cons__paths_src_node_from_path_node(bctx, path_node);
        file_idx = src_node->idx;
      }
      
      ptr->file_idx = file_idx;
      ptr->line = loose_udt->line;
      ptr->col = loose_udt->col;
    }
    
    // all iterators should end at the same time
    Assert(loose_udt == 0);
    Assert(ptr == opl);
    Assert(member_ptr == member_opl);
    Assert(enum_member_ptr == enum_member_opl);
  }
  
  
  // fill result
  CONS__TypeData *result = push_array(arena, CONS__TypeData, 1);
  result->type_nodes = type_nodes;
  result->type_node_count = type_count;
  result->udts = udts;
  result->udt_count = udt_count;
  result->members = members;
  result->member_count = member_count;
  result->enum_members = enum_members;
  result->enum_member_count = enum_member_count;
  
  scratch_end(scratch);
  ProfEnd();
  
  return(result);
}

static U32*
cons__idx_run_from_types(Arena *arena, CONS_Type **types, U32 count){
  U32 *result = push_array(arena, U32, count);
  for (U32 i = 0; i < count; i += 1){
    result[i] = types[i]->idx;
  }
  return(result);
}

//- cons serializer for symbols
static CONS__SymbolData*
cons__symbol_data_combine(Arena *arena, CONS_Root *root, CONS__BakeCtx *bctx){
  ProfBegin("cons__symbol_data_combine");
  Temp scratch = scratch_begin(&arena, 1);
  
  // count symbol kinds
  U32 globalvar_count = 1 + root->symbol_kind_counts[CONS_SymbolKind_GlobalVariable];
  U32 threadvar_count = 1 + root->symbol_kind_counts[CONS_SymbolKind_ThreadVariable];
  U32 procedure_count = 1 + root->symbol_kind_counts[CONS_SymbolKind_Procedure];
  
  // allocate symbol arrays
  RADDBG_GlobalVariable *global_variables =
    push_array(arena, RADDBG_GlobalVariable, globalvar_count);
  
  RADDBG_ThreadVariable *thread_variables =
    push_array(arena, RADDBG_ThreadVariable, threadvar_count);
  
  RADDBG_Procedure *procedures = push_array(arena, RADDBG_Procedure, procedure_count);
  
  // fill symbol arrays
  {
    RADDBG_GlobalVariable *global_ptr = global_variables;
    RADDBG_ThreadVariable *thread_local_ptr = thread_variables;
    RADDBG_Procedure *procedure_ptr = procedures;
    
    // nils
    global_ptr += 1;
    thread_local_ptr += 1;
    procedure_ptr += 1;
    
    // symbol nodes
    for (CONS_Symbol *node = root->first_symbol;
         node != 0;
         node = node->next_order){
      U32 name_string_idx = cons__string(bctx, node->name);
      U32 link_name_string_idx = cons__string(bctx, node->link_name);
      U32 type_idx = node->type->idx;
      
      RADDBG_LinkFlags link_flags = 0;
      U32 container_idx = 0;
      {      
        if (node->is_extern){
          link_flags |= RADDBG_LinkFlag_External;
        }
        if (node->container_symbol != 0){
          container_idx = node->container_symbol->idx;
          link_flags |= RADDBG_LinkFlag_ProcScoped;
        }
        else if (node->container_type != 0 && node->container_type->udt != 0){
          container_idx = node->container_type->udt->idx;
          link_flags |= RADDBG_LinkFlag_TypeScoped;
        }
      }
      
      switch (node->kind){
        default:{}break;
        
        case CONS_SymbolKind_GlobalVariable:
        {
          global_ptr->name_string_idx = name_string_idx;
          global_ptr->link_flags = link_flags;
          global_ptr->voff = node->offset;
          global_ptr->type_idx = type_idx;
          global_ptr->container_idx = container_idx;
          global_ptr += 1;
        }break;
        
        case CONS_SymbolKind_ThreadVariable:
        {
          thread_local_ptr->name_string_idx = name_string_idx;
          thread_local_ptr->link_flags = link_flags;
          thread_local_ptr->tls_off = (U32)node->offset;
          thread_local_ptr->type_idx = type_idx;
          thread_local_ptr->container_idx = container_idx;
          thread_local_ptr += 1;
        }break;
        
        case CONS_SymbolKind_Procedure:
        {
          procedure_ptr->name_string_idx = name_string_idx;
          procedure_ptr->link_name_string_idx = link_name_string_idx;
          procedure_ptr->link_flags = link_flags;
          procedure_ptr->type_idx = type_idx;
          procedure_ptr->root_scope_idx = node->root_scope->idx;
          procedure_ptr->container_idx = container_idx;
          procedure_ptr += 1;
        }break;
      }
    }
    
    Assert(global_ptr - global_variables == globalvar_count);
    Assert(thread_local_ptr - thread_variables == threadvar_count);
    Assert(procedure_ptr - procedures == procedure_count);
  }
  
  // global vmap
  CONS__VMap *global_vmap = 0;
  {
    // count necessary markers
    U32 marker_count = globalvar_count*2;
    
    // fill markers
    CONS__SortKey    *keys = push_array_no_zero(scratch.arena, CONS__SortKey, marker_count);
    CONS__VMapMarker *markers = push_array_no_zero(scratch.arena, CONS__VMapMarker, marker_count);
    
    CONS__SortKey *key_ptr = keys;
    CONS__VMapMarker *marker_ptr = markers;
    
    // real globals
    for (CONS_Symbol *node = root->first_symbol;
         node != 0;
         node = node->next_order){
      if (node->kind == CONS_SymbolKind_GlobalVariable){
        U32 global_idx = node->idx;
        
        U64 first = node->offset;
        U64 opl   = first + node->type->byte_size;
        
        key_ptr->key = first;
        key_ptr->val = marker_ptr;
        marker_ptr->idx = global_idx;
        marker_ptr->begin_range = 1;
        key_ptr += 1;
        marker_ptr += 1;
        
        key_ptr->key = opl;
        key_ptr->val = marker_ptr;
        marker_ptr->idx = global_idx;
        marker_ptr->begin_range = 0;
        key_ptr += 1;
        marker_ptr += 1;
      }
    }
    
    // nil global
    {
      U32 global_idx = 0;
      
      U64 first = 0;
      U64 opl   = max_U64;
      
      key_ptr->key = first;
      key_ptr->val = marker_ptr;
      marker_ptr->idx = global_idx;
      marker_ptr->begin_range = 1;
      key_ptr += 1;
      marker_ptr += 1;
      
      key_ptr->key = opl;
      key_ptr->val = marker_ptr;
      marker_ptr->idx = global_idx;
      marker_ptr->begin_range = 0;
      key_ptr += 1;
      marker_ptr += 1;
    }
    
    // assert we filled all the markers
    Assert(key_ptr - keys == marker_count &&
           marker_ptr - markers == marker_count);
    
    // construct vmap
    global_vmap = cons__vmap_from_markers(arena, markers, keys, marker_count);
  }
  
  // allocate scope array
  
  // (assert there is a nil scope)
  Assert(root->first_scope != 0 &&
         root->first_scope->symbol == 0 &&
         root->first_scope->first_child == 0 &&
         root->first_scope->next_sibling == 0 &&
         root->first_scope->range_count == 0);
  
  U32 scope_count = root->scope_count;
  RADDBG_Scope *scopes = push_array(arena, RADDBG_Scope, scope_count);
  
  U32 scope_voff_count = root->scope_voff_count;
  U64 *scope_voffs = push_array(arena, U64, scope_voff_count);
  
  U32 local_count = root->local_count;
  RADDBG_Local *locals = push_array(arena, RADDBG_Local, local_count);
  
  U32 location_block_count = root->location_count;
  RADDBG_LocationBlock *location_blocks =
    push_array(arena, RADDBG_LocationBlock, location_block_count);
  
  String8List location_data = {0};
  
  // iterate scopes, locals, and locations
  //  fill scope voffs, locals, and location information
  {
    RADDBG_Scope *scope_ptr = scopes;
    U64 *scope_voff_ptr = scope_voffs;
    RADDBG_Local *local_ptr = locals;
    RADDBG_LocationBlock *location_block_ptr = location_blocks;
    
    for (CONS_Scope *node = root->first_scope;
         node != 0;
         node = node->next_order, scope_ptr += 1){
      
      // emit voffs
      U32 voff_first = (U32)(scope_voff_ptr - scope_voffs);
      for (CONS__VOffRange *range = node->first_range;
           range != 0;
           range = range->next){
        *scope_voff_ptr = range->voff_first;
        scope_voff_ptr += 1;
        *scope_voff_ptr = range->voff_opl;
        scope_voff_ptr += 1;
      }
      U32 voff_opl = (U32)(scope_voff_ptr - scope_voffs);
      
      // emit locals
      U32 scope_local_count = node->local_count;
      U32 scope_local_first = (U32)(local_ptr - locals);
      for (CONS_Local *slocal = node->first_local;
           slocal != 0;
           slocal = slocal->next, local_ptr += 1){
        local_ptr->kind = slocal->kind;
        local_ptr->name_string_idx = cons__string(bctx, slocal->name);
        local_ptr->type_idx = slocal->type->idx;
        
        CONS_LocationSet *locset = slocal->locset;
        if (locset != 0){
          U32 location_first = (U32)(location_block_ptr - location_blocks);
          U32 location_opl   = location_first + locset->location_case_count;
          local_ptr->location_first = location_first;
          local_ptr->location_opl   = location_opl;
          
          for (CONS__LocationCase *location_case = locset->first_location_case;
               location_case != 0;
               location_case = location_case->next){
            location_block_ptr->scope_off_first = location_case->voff_first;
            location_block_ptr->scope_off_opl   = location_case->voff_opl;
            location_block_ptr->location_data_off = location_data.total_size;
            location_block_ptr += 1;
            
            CONS_Location *location = location_case->location;
            if (location == 0){
              U64 data = 0;
              str8_serial_push_align(scratch.arena, &location_data, 8);
              str8_serial_push_data(scratch.arena, &location_data, &data, 1);
            }
            else{
              switch (location->kind){
                default:
                {
                  U64 data = 0;
                  str8_serial_push_align(scratch.arena, &location_data, 8);
                  str8_serial_push_data(scratch.arena, &location_data, &data, 1);
                }break;
                
                case RADDBG_LocationKind_AddrBytecodeStream:
                case RADDBG_LocationKind_ValBytecodeStream:
                {
                  str8_list_push(scratch.arena, &location_data, push_str8_copy(scratch.arena, str8_struct(&location->kind)));
                  for (CONS_EvalBytecodeOp *op_node = location->bytecode.first_op;
                       op_node != 0;
                       op_node = op_node->next){
                    U8 op_data[9];
                    op_data[0] = op_node->op;
                    MemoryCopy(op_data + 1, &op_node->p, op_node->p_size);
                    String8 op_data_str = str8(op_data, 1 + op_node->p_size);
                    str8_list_push(scratch.arena, &location_data, push_str8_copy(scratch.arena, op_data_str));
                  }
                  {
                    U64 data = 0;
                    String8 data_str = str8((U8 *)&data, 1);
                    str8_list_push(scratch.arena, &location_data, push_str8_copy(scratch.arena, data_str));
                  }
                }break;
                
                case RADDBG_LocationKind_AddrRegisterPlusU16:
                case RADDBG_LocationKind_AddrAddrRegisterPlusU16:
                {
                  RADDBG_LocationRegisterPlusU16 loc = {0};
                  loc.kind = location->kind;
                  loc.register_code = location->register_code;
                  loc.offset = location->offset;
                  str8_list_push(scratch.arena, &location_data, push_str8_copy(scratch.arena, str8_struct(&loc)));
                }break;
                
                case RADDBG_LocationKind_ValRegister:
                {
                  RADDBG_LocationRegister loc = {0};
                  loc.kind = location->kind;
                  loc.register_code = location->register_code;
                  str8_list_push(scratch.arena, &location_data, push_str8_copy(scratch.arena, str8_struct(&loc)));
                }break;
              }
            }
          }
          
          Assert(location_block_ptr - location_blocks == location_opl);
        }
      }
      
      Assert(local_ptr - locals == scope_local_first + scope_local_count);
      
      // emit scope
      scope_ptr->proc_idx = (node->symbol == 0)?0:node->symbol->idx;
      scope_ptr->parent_scope_idx = (node->parent_scope == 0)?0:node->parent_scope->idx;
      scope_ptr->first_child_scope_idx = (node->first_child == 0)?0:node->first_child->idx;
      scope_ptr->next_sibling_scope_idx = (node->next_sibling == 0)?0:node->next_sibling->idx;
      scope_ptr->voff_range_first = voff_first;
      scope_ptr->voff_range_opl = voff_opl;
      scope_ptr->local_first = scope_local_first;
      scope_ptr->local_count = scope_local_count;
      
      // TODO(allen): 
      //scope_ptr->static_local_idx_run_first = ;
      //scope_ptr->static_local_count = ;
    }
    
    Assert(scope_ptr - scopes == scope_count);
    Assert(local_ptr - locals == local_count);
  }
  
  // flatten location data
  String8 location_data_str = str8_list_join(arena, &location_data, 0);
  
  // scope vmap
  CONS__VMap *scope_vmap = 0;
  {
    // count necessary markers
    U32 marker_count = scope_voff_count;
    
    // fill markers
    CONS__SortKey    *keys = push_array_no_zero(scratch.arena, CONS__SortKey, marker_count);
    CONS__VMapMarker *markers = push_array_no_zero(scratch.arena, CONS__VMapMarker, marker_count);
    
    CONS__SortKey *key_ptr = keys;
    CONS__VMapMarker *marker_ptr = markers;
    
    for (CONS_Scope *node = root->first_scope;
         node != 0;
         node = node->next_order){
      U32 scope_idx = node->idx;
      
      for (CONS__VOffRange *range = node->first_range;
           range != 0;
           range = range->next){
        key_ptr->key = range->voff_first;
        key_ptr->val = marker_ptr;
        marker_ptr->idx = scope_idx;
        marker_ptr->begin_range = 1;
        key_ptr += 1;
        marker_ptr += 1;
        
        key_ptr->key = range->voff_opl;
        key_ptr->val = marker_ptr;
        marker_ptr->idx = scope_idx;
        marker_ptr->begin_range = 0;
        key_ptr += 1;
        marker_ptr += 1;
      }
    }
    
    scope_vmap = cons__vmap_from_markers(arena, markers, keys, marker_count);
  }
  
  // fill result
  CONS__SymbolData *result = push_array(arena, CONS__SymbolData, 1);
  result->global_variables = global_variables;
  result->global_variable_count = globalvar_count;
  result->global_vmap = global_vmap;
  result->thread_variables = thread_variables;
  result->thread_variable_count = threadvar_count;
  result->procedures = procedures;
  result->procedure_count = procedure_count;
  result->scopes = scopes;
  result->scope_count = scope_count;
  result->scope_voffs = scope_voffs;
  result->scope_voff_count = scope_voff_count;
  result->scope_vmap = scope_vmap;
  result->locals = locals;
  result->local_count = local_count;
  result->location_blocks = location_blocks;
  result->location_block_count = location_block_count;
  result->location_data = location_data_str.str;
  result->location_data_size = location_data_str.size;
  
  scratch_end(scratch);
  ProfEnd();
  
  return(result);
}

//- cons serializer for name maps

static CONS__NameMapBaked*
cons__name_map_bake(Arena *arena, CONS_Root *root, CONS__BakeCtx *bctx, CONS__NameMap *map){
  Temp scratch = scratch_begin(&arena, 1);
  
  U32 bucket_count = map->name_count;
  U32 node_count = map->name_count;
  
  // setup the final bucket layouts
  CONS__NameMapSemiBucket *sbuckets = push_array(scratch.arena, CONS__NameMapSemiBucket, bucket_count);
  for (CONS__NameMapNode *node = map->first;
       node != 0;
       node = node->order_next){
    U64 hash = raddbg_hash(node->string.str, node->string.size);
    U64 bi = hash%bucket_count;
    CONS__NameMapSemiNode *snode = push_array(scratch.arena, CONS__NameMapSemiNode, 1);
    SLLQueuePush(sbuckets[bi].first, sbuckets[bi].last, snode);
    snode->node = node;
    sbuckets[bi].count += 1;
  }
  
  // allocate tables
  RADDBG_NameMapBucket *buckets = push_array(arena, RADDBG_NameMapBucket, bucket_count);
  RADDBG_NameMapNode *nodes = push_array_no_zero(arena, RADDBG_NameMapNode, node_count);
  
  // convert to serialized buckets & nodes
  {
    RADDBG_NameMapBucket *bucket_ptr = buckets;
    RADDBG_NameMapNode *node_ptr = nodes;
    for (U32 i = 0; i < bucket_count; i += 1, bucket_ptr += 1){
      bucket_ptr->first_node = (U32)(node_ptr - nodes);
      bucket_ptr->node_count = sbuckets[i].count;
      
      for (CONS__NameMapSemiNode *snode = sbuckets[i].first;
           snode != 0;
           snode = snode->next){
        CONS__NameMapNode *node = snode->node;
        
        // cons name and index(es)
        U32 string_idx = cons__string(bctx, node->string);
        U32 match_count = node->idx_count;
        U32 idx = 0;
        if (match_count == 1){
          idx = node->idx_first->idx[0];
        }
        else{
          Temp temp = temp_begin(scratch.arena);
          U32 *idx_run = push_array_no_zero(temp.arena, U32, match_count);
          U32 *idx_ptr = idx_run;
          for (CONS__NameMapIdxNode *idxnode = node->idx_first;
               idxnode != 0;
               idxnode = idxnode->next){
            for (U32 i = 0; i < ArrayCount(idxnode->idx); i += 1){
              if (idxnode->idx[i] == 0){
                goto dblbreak;
              }
              *idx_ptr = idxnode->idx[i];
              idx_ptr += 1;
            }
          }
          dblbreak:;
          Assert(idx_ptr == idx_run + match_count);
          idx = cons__idx_run(bctx, idx_run, match_count);
          temp_end(temp);
        }
        
        // write to node
        node_ptr->string_idx = string_idx;
        node_ptr->match_count = match_count;
        node_ptr->match_idx_or_idx_run_first = idx;
        node_ptr += 1;
      }
    }
    Assert(node_ptr - nodes == node_count);
  }
  
  scratch_end(scratch);
  
  CONS__NameMapBaked *result = push_array(arena, CONS__NameMapBaked, 1);
  result->buckets = buckets;
  result->nodes = nodes;
  result->bucket_count = bucket_count;
  result->node_count = node_count;
  return(result);
}
