// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ Program Parameters Parser

static PDBCONV_Params*
pdb_convert_params_from_cmd_line(Arena *arena, CmdLine *cmdline){
  PDBCONV_Params *result = push_array(arena, PDBCONV_Params, 1);
  
  // get input pdb
  {
    String8 input_name = cmd_line_string(cmdline, str8_lit("pdb"));
    if (input_name.size == 0){
      str8_list_push(arena, &result->errors,
                     str8_lit("missing required parameter '--pdb:<pdb_file>'"));
    }
    
    if (input_name.size > 0){
      String8 input_data = os_data_from_file_path(arena, input_name);
      
      if (input_data.size == 0){
        str8_list_pushf(arena, &result->errors,
                        "could not load input file '%.*s'", str8_varg(input_name));
      }
      
      if (input_data.size != 0){
        result->input_pdb_name = input_name;
        result->input_pdb_data = input_data;
      }
    }
  }
  
  // get input exe
  {
    String8 input_name = cmd_line_string(cmdline, str8_lit("exe"));
    if (input_name.size > 0){
      String8 input_data = os_data_from_file_path(arena, input_name);
      
      if (input_data.size == 0){
        str8_list_pushf(arena, &result->errors,
                        "could not load input file '%.*s'", str8_varg(input_name));
      }
      
      if (input_data.size != 0){
        result->input_exe_name = input_name;
        result->input_exe_data = input_data;
      }
    }
  }
  
  // get output name
  {
    result->output_name = cmd_line_string(cmdline, str8_lit("out"));
  }
  
  // error options
  if (cmd_line_has_flag(cmdline, str8_lit("hide_errors"))){
    String8List vals = cmd_line_strings(cmdline, str8_lit("hide_errors"));
    
    // if no values - set all to hidden
    if (vals.node_count == 0){
      B8 *ptr  = (B8*)&result->hide_errors;
      B8 *opl = ptr + sizeof(result->hide_errors);
      for (;ptr < opl; ptr += 1){
        *ptr = 1;
      }
    }
    
    // for each explicit value set the corresponding flag to hidden
    for (String8Node *node = vals.first;
         node != 0;
         node = node->next){
      if (str8_match(node->string, str8_lit("input"), 0)){
        result->hide_errors.input = 1;
      }
      else if (str8_match(node->string, str8_lit("output"), 0)){
        result->hide_errors.output = 1;
      }
      else if (str8_match(node->string, str8_lit("parsing"), 0)){
        result->hide_errors.parsing = 1;
      }
      else if (str8_match(node->string, str8_lit("converting"), 0)){
        result->hide_errors.converting = 1;
      }
    }
    
  }
  
  // dump options
  if (cmd_line_has_flag(cmdline, str8_lit("dump"))){
    result->dump = 1;
    
    String8List vals = cmd_line_strings(cmdline, str8_lit("dump"));
    if (vals.first == 0){
      B8 *ptr = &result->dump__first;
      for (; ptr < &result->dump__last; ptr += 1){
        *ptr = 1;
      }
    }
    else{
      for (String8Node *node = vals.first;
           node != 0;
           node = node->next){
        if (str8_match(node->string, str8_lit("coff_sections"), 0)){
          result->dump_coff_sections = 1;
        }
        else if (str8_match(node->string, str8_lit("msf"), 0)){
          result->dump_msf = 1;
        }
        else if (str8_match(node->string, str8_lit("sym"), 0)){
          result->dump_sym = 1;
        }
        else if (str8_match(node->string, str8_lit("tpi_hash"), 0)){
          result->dump_tpi_hash = 1;
        }
        else if (str8_match(node->string, str8_lit("leaf"), 0)){
          result->dump_leaf = 1;
        }
        else if (str8_match(node->string, str8_lit("c13"), 0)){
          result->dump_c13 = 1;
        }
        else if (str8_match(node->string, str8_lit("contributions"), 0)){
          result->dump_contributions = 1;
        }
        else if (str8_match(node->string, str8_lit("table_diagnostics"), 0)){
          result->dump_table_diagnostics = 1;
        }
      }
    }
  }
  
  return(result);
}

////////////////////////////////
//~ PDB Type & Symbol Info Translation Helpers

//- rjf: pdb conversion context creation

static PDBCONV_Ctx *
pdbconv_ctx_alloc(PDBCONV_CtxParams *params, RADDBGIC_Root *out_root)
{
  Arena *arena = arena_alloc();
  PDBCONV_Ctx *pdb_ctx = push_array(arena, PDBCONV_Ctx, 1);
  pdb_ctx->arena = arena;
  pdb_ctx->arch = params->arch;
  pdb_ctx->addr_size = raddbgi_addr_size_from_arch(pdb_ctx->arch);
  pdb_ctx->hash = params->tpi_hash;
  pdb_ctx->leaf = params->tpi_leaf;
  pdb_ctx->sections = params->sections->sections;
  pdb_ctx->section_count = params->sections->count;
  pdb_ctx->root = out_root;
#define BKTCOUNT(x) ((x)?(u64_up_to_pow2(x)):(4096))
  pdb_ctx->fwd_map.buckets_count        = BKTCOUNT(params->fwd_map_bucket_count);
  pdb_ctx->frame_proc_map.buckets_count = BKTCOUNT(params->frame_proc_map_bucket_count);
  pdb_ctx->known_globals.buckets_count  = BKTCOUNT(params->known_global_map_bucket_count);
  pdb_ctx->link_names.buckets_count     = BKTCOUNT(params->link_name_map_bucket_count);
#undef BKTCOUNT
  pdb_ctx->fwd_map.buckets = push_array(pdb_ctx->arena, PDBCONV_FwdNode *, pdb_ctx->fwd_map.buckets_count);
  pdb_ctx->frame_proc_map.buckets = push_array(pdb_ctx->arena, PDBCONV_FrameProcNode *, pdb_ctx->frame_proc_map.buckets_count);
  pdb_ctx->known_globals.buckets = push_array(pdb_ctx->arena, PDBCONV_KnownGlobalNode *, pdb_ctx->known_globals.buckets_count);
  pdb_ctx->link_names.buckets = push_array(pdb_ctx->arena, PDBCONV_LinkNameNode *, pdb_ctx->link_names.buckets_count);
  return pdb_ctx;
}

//- pdb types and symbols

static void
pdbconv_types_and_symbols(PDBCONV_Ctx *pdb_ctx, PDBCONV_TypesSymbolsParams *params)
{
  ProfBeginFunction();
  
  // convert types
  pdbconv_type_cons_main_passes(pdb_ctx);
  if (params->sym != 0){
    pdbconv_gather_link_names(pdb_ctx, params->sym);
    pdbconv_symbol_cons(pdb_ctx, params->sym, 0);
  }
  U64 unit_count = params->unit_count;
  for (U64 i = 0; i < unit_count; i += 1){
    CV_SymParsed *unit_sym = params->sym_for_unit[i];
    pdbconv_symbol_cons(pdb_ctx, unit_sym, 1 + i);
  }
  
  ProfEnd();
}

//- decoding helpers

static U32
pdbconv_u32_from_numeric(PDBCONV_Ctx *ctx, CV_NumericParsed *num){
  U64 n_u64 = cv_u64_from_numeric(num);
  U32 n_u32 = (U32)n_u64;
  if (n_u64 > 0xFFFFFFFF){
    raddbgic_push_errorf(ctx->root, "constant too large");
    n_u32 = 0;
  }
  return(n_u32);
}

static COFF_SectionHeader*
pdbconv_sec_header_from_sec_num(PDBCONV_Ctx *ctx, U32 sec_num){
  COFF_SectionHeader *result = 0;
  if (0 < sec_num && sec_num <= ctx->section_count){
    result = ctx->sections + sec_num - 1;
  }
  return(result);
}

//- type info

static void
pdbconv_type_cons_main_passes(PDBCONV_Ctx *ctx){
  ProfBeginFunction();
  CV_TypeId itype_first = ctx->leaf->itype_first;
  CV_TypeId itype_opl = ctx->leaf->itype_opl;
  
  // setup variadic itype -> node
  ProfScope("setup variadic itype -> node")
  {
    RADDBGIC_Type *variadic_type = raddbgic_type_variadic(ctx->root);
    RADDBGIC_Reservation *res = raddbgic_type_reserve_id(ctx->root, CV_TypeId_Variadic, CV_TypeId_Variadic);
    raddbgic_type_fill_id(ctx->root, res, variadic_type);
  }
  
  // resolve forward references
  ProfScope("resolve forward references")
  {
    for (CV_TypeId itype = itype_first; itype < itype_opl; itype += 1){
      pdbconv_type_resolve_fwd(ctx, itype);
    }
  }
  
  // construct type info
  ProfScope("construct type info")
  {
    for (CV_TypeId itype = itype_first; itype < itype_opl; itype += 1){
      pdbconv_type_resolve_itype(ctx, itype);
    }
  }
  
  // construct member info
  ProfScope("construct member info")
  {
    for (PDBCONV_TypeRev *rev = ctx->member_revisit_first;
         rev != 0;
         rev = rev->next){
      pdbconv_type_equip_members(ctx, rev->owner_type, rev->field_itype);
    }
  }
  
  // construct enum info
  ProfScope("construct enum info")
  {
    for (PDBCONV_TypeRev *rev = ctx->enum_revisit_first;
         rev != 0;
         rev = rev->next){
      pdbconv_type_equip_enumerates(ctx, rev->owner_type, rev->field_itype);
    }
  }
  
  // TODO(allen): equip udts with location information
  ProfEnd();
}

static CV_TypeId
pdbconv_type_resolve_fwd(PDBCONV_Ctx *ctx, CV_TypeId itype){
  ProfBeginFunction();
  Assert(ctx->leaf->itype_first <= itype && itype < ctx->leaf->itype_opl);
  
  CV_TypeId result = 0;
  
  CV_RecRange *range = &ctx->leaf->leaf_ranges.ranges[itype - ctx->leaf->itype_first];
  String8 data = ctx->leaf->data;
  if (range->off + range->hdr.size <= data.size){
    U8 *first = data.str + range->off + 2;
    U64 cap = range->hdr.size - 2;
    
    // figure out if this itype resolves to another
    switch (range->hdr.kind){
      default:break;
      
      case CV_LeafKind_CLASS:
      case CV_LeafKind_STRUCTURE:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafStruct) <= cap){
          CV_LeafStruct *lf_struct = (CV_LeafStruct*)first;
          
          // size
          U8 *numeric_ptr = (U8*)(lf_struct + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          
          // name
          U8 *name_ptr = numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // unique name
          U8 *unique_name_ptr = name_ptr + name.size + 1;
          String8 unique_name = str8_cstring_capped((char*)unique_name_ptr, first + cap);
          
          if (lf_struct->props & CV_TypeProp_FwdRef){
            B32 do_unique_name_lookup = ((lf_struct->props & CV_TypeProp_Scoped) != 0) &&
            ((lf_struct->props & CV_TypeProp_HasUniqueName) != 0);
            if (do_unique_name_lookup){
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, unique_name, 1);
            }
            else{
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, name, 0);
            }
          }
        }
      }break;
      
      case CV_LeafKind_CLASS2:
      case CV_LeafKind_STRUCT2:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafStruct2) <= cap){
          CV_LeafStruct2 *lf_struct = (CV_LeafStruct2*)first;
          
          // size
          U8 *numeric_ptr = (U8*)(lf_struct + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          
          // name
          U8 *name_ptr = (U8 *)numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // unique name
          U8 *unique_name_ptr = name_ptr + name.size + 1;
          String8 unique_name = str8_cstring_capped((char*)unique_name_ptr, first + cap);
          
          if (lf_struct->props & CV_TypeProp_FwdRef){
            B32 do_unique_name_lookup = ((lf_struct->props & CV_TypeProp_Scoped) != 0) &&
            ((lf_struct->props & CV_TypeProp_HasUniqueName) != 0);
            if (do_unique_name_lookup){
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, unique_name, 1);
            }
            else{
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, name, 0);
            }
          }
        }
      }break;
      
      case CV_LeafKind_UNION:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafUnion) <= cap){
          CV_LeafUnion *lf_union = (CV_LeafUnion*)first;
          
          // size
          U8 *numeric_ptr = (U8*)(lf_union + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          
          // name
          U8 *name_ptr = numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // unique name
          U8 *unique_name_ptr = name_ptr + name.size + 1;
          String8 unique_name = str8_cstring_capped((char*)unique_name_ptr, first + cap);
          
          if (lf_union->props & CV_TypeProp_FwdRef){
            B32 do_unique_name_lookup = ((lf_union->props & CV_TypeProp_Scoped) != 0) &&
            ((lf_union->props & CV_TypeProp_HasUniqueName) != 0);
            if (do_unique_name_lookup){
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, unique_name, 1);
            }
            else{
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, name, 0);
            }
          }
        }
      }break;
      
      case CV_LeafKind_ENUM:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafEnum) <= cap){
          CV_LeafEnum *lf_enum = (CV_LeafEnum*)first;
          
          // name
          U8 *name_ptr = (U8*)(lf_enum + 1);
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // unique name
          U8 *unique_name_ptr = name_ptr + name.size + 1;
          String8 unique_name = str8_cstring_capped((char*)unique_name_ptr, first + cap);
          
          if (lf_enum->props & CV_TypeProp_FwdRef){
            B32 do_unique_name_lookup = ((lf_enum->props & CV_TypeProp_Scoped) != 0) &&
            ((lf_enum->props & CV_TypeProp_HasUniqueName) != 0);
            if (do_unique_name_lookup){
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, unique_name, 1);
            }
            else{
              result = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, name, 0);
            }
          }
        }
      }break;
    }
  }
  
  // save in map
  if (result != 0){
    pdbconv_type_fwd_map_set(ctx->arena, &ctx->fwd_map, itype, result);
  }
  
  ProfEnd();
  return(result);
}

static RADDBGIC_Type*
pdbconv_type_resolve_itype(PDBCONV_Ctx *ctx, CV_TypeId itype){
  B32 is_basic = (itype < 0x1000);
  
  // convert fwd references to real types
  if(!is_basic)
  {
    CV_TypeId resolved_itype = pdbconv_type_fwd_map_get(&ctx->fwd_map, itype);
    if(resolved_itype != 0)
    {
      itype = resolved_itype;
    }
  }
  
  // type handle from id
  RADDBGIC_Type *result = raddbgic_type_from_id(ctx->root, itype, itype);
  
  // basic type
  if(result == 0 && is_basic)
  {
    result = pdbconv_type_cons_basic(ctx, itype);
  }
  
  // leaf decode
  if(result == 0 && (ctx->leaf->itype_first <= itype && itype < ctx->leaf->itype_opl))
  {
    result = pdbconv_type_cons_leaf_record(ctx, itype);
  }
  
  // never return null, return "nil" instead
  if(result == 0)
  {
    result = raddbgic_type_nil(ctx->root);
  }
  
  return(result);
}

static void
pdbconv_type_equip_members(PDBCONV_Ctx *ctx, RADDBGIC_Type *owner_type, CV_TypeId field_itype){
  Temp scratch = scratch_begin(0, 0);
  
  String8 data = ctx->leaf->data;
  
  // field stack
  // TODO(allen): add notes about field tasks
  struct FieldTask{
    struct FieldTask *next;
    CV_TypeId itype;
  };
  struct FieldTask *handled = 0;
  struct FieldTask *todo = 0;
  {
    struct FieldTask *task = push_array(scratch.arena, struct FieldTask, 1);
    SLLStackPush(todo, task);
    task->itype = field_itype;
  }
  
  for (;;){
    // exit condition
    if (todo == 0){
      break;
    }
    
    // determine itype
    CV_TypeId field_itype = todo->itype;
    {
      struct FieldTask *task = todo;
      SLLStackPop(todo);
      SLLStackPush(handled, task);
    }
    
    // get leaf range
    // TODO(allen): error if this itype is bad
    U8 *first = 0;
    U64 cap = 0;
    if (ctx->leaf->itype_first <= field_itype && field_itype < ctx->leaf->itype_opl){
      CV_RecRange *range = &ctx->leaf->leaf_ranges.ranges[field_itype - ctx->leaf->itype_first];
      // check valid arglist
      if (range->hdr.kind == CV_LeafKind_FIELDLIST &&
          range->off + range->hdr.size <= data.size){
        first = data.str + range->off + 2;
        cap = range->hdr.size - 2;
      }
    }
    
    U64 cursor = 0;
    for (;cursor + sizeof(CV_LeafKind) <= cap;){
      CV_LeafKind field_kind = *(CV_LeafKind*)(first + cursor);
      
      U64 list_item_off = cursor + 2;
      // if we hit an error or forget to set next cursor for a case
      // default to exiting the loop
      U64 list_item_opl_off = cap;
      
      switch (field_kind){
        case CV_LeafKind_INDEX:
        {
          // TODO(allen): error if bad range
          if (list_item_off + sizeof(CV_LeafIndex) <= cap){
            // compute whole layout
            CV_LeafIndex *index = (CV_LeafIndex*)(first + list_item_off);
            
            list_item_opl_off = list_item_off + sizeof(*index);
            
            // create new todo task
            CV_TypeId new_itype = index->itype;
            B32 is_new = 1;
            for (struct FieldTask *task = handled;
                 task != 0;
                 task = task->next){
              if (task->itype == new_itype){
                is_new = 0;
                break;
              }
            }
            if (is_new){
              struct FieldTask *task = push_array(scratch.arena, struct FieldTask, 1);
              SLLStackPush(todo, task);
              task->itype = new_itype;
            }
          }
        }break;
        
        case CV_LeafKind_MEMBER:
        {
          // TODO(allen): error if bad range
          if (list_item_off + sizeof(CV_LeafMember) <= cap){
            // compute whole layout
            CV_LeafMember *member = (CV_LeafMember*)(first + list_item_off);
            
            U64 offset_off = list_item_off + sizeof(*member);
            CV_NumericParsed offset = cv_numeric_from_data_range(first + offset_off, first + cap);
            
            U64 name_off = offset_off + offset.encoded_size;
            String8 name = str8_cstring_capped(first + name_off, first + cap);
            
            list_item_opl_off = name_off + name.size + 1;
            
            // emit member
            RADDBGIC_Type *mem_type = pdbconv_type_resolve_itype(ctx, member->itype);
            U32 offset_u32 = pdbconv_u32_from_numeric(ctx, &offset);
            raddbgic_type_add_member_data_field(ctx->root, owner_type, name, mem_type, offset_u32);
          }
        }break;
        
        case CV_LeafKind_STMEMBER:
        {
          // TODO(allen): error if bad range
          if (list_item_off + sizeof(CV_LeafStMember) <= cap){
            // compute whole layout
            CV_LeafStMember *stmember = (CV_LeafStMember*)(first + list_item_off);
            
            U64 name_off = list_item_off + sizeof(*stmember);
            String8 name = str8_cstring_capped(first + name_off, first + cap);
            
            list_item_opl_off = name_off + name.size + 1;
            
            // TODO(allen): handle attribs
            
            // emit member
            RADDBGIC_Type *mem_type = pdbconv_type_resolve_itype(ctx, stmember->itype);
            raddbgic_type_add_member_static_data(ctx->root, owner_type, name, mem_type);
          }
        }break;
        
        case CV_LeafKind_METHOD:
        {
          // TODO(allen): error if bad range
          if (list_item_off + sizeof(CV_LeafMethod) <= cap){
            // compute whole layout
            CV_LeafMethod *method = (CV_LeafMethod*)(first + list_item_off);
            
            U64 name_off = list_item_off + sizeof(*method);
            String8 name = str8_cstring_capped(first + name_off, first + cap);
            
            list_item_opl_off = name_off + name.size + 1;
            
            // extract method list
            U8 *first = 0;
            U64 cap = 0;
            
            // TODO(allen): error if bad itype
            CV_TypeId methodlist_itype = method->list_itype;
            if (ctx->leaf->itype_first <= methodlist_itype &&
                methodlist_itype < ctx->leaf->itype_opl){
              CV_RecRange *range = &ctx->leaf->leaf_ranges.ranges[methodlist_itype - ctx->leaf->itype_first];
              
              // check valid methodlist
              if (range->hdr.kind == CV_LeafKind_METHODLIST &&
                  range->off + range->hdr.size <= data.size){
                first = data.str + range->off + 2;
                cap = range->hdr.size - 2;
              }
            }
            
            // emit loop
            U64 cursor = 0;
            for (;cursor + sizeof(CV_LeafMethodListMember) <= cap;){
              CV_LeafMethodListMember *method = (CV_LeafMethodListMember*)(first + cursor);
              
              CV_MethodProp prop = CV_FieldAttribs_ExtractMethodProp(method->attribs);
              
              // TODO(allen): PROBLEM
              // We only get offsets for virtual functions (the "vbaseoff") from
              // "Intro" and "PureIntro". In C++ inheritance, when we have a chain
              // of inheritance (let's just talk single inheritance for now) the
              // first class in the chain that introduces a new virtual function
              // has this "Intro" method. If a later class in the chain redefines
              // the virtual function it only has a "Virtual" method which does
              // not update the offset. There is a "Virtual" and "PureVirtual"
              // variant of "Virtual". The "Pure" in either case means there
              // is no concrete procedure. When there is no "Pure" the method
              // should have a corresponding procedure symbol id.
              //
              // The issue is we will want to mark all of our virtual methods as
              // virtual and give them an offset, but that means we have to do
              // some extra figuring to propogate offsets from "Intro" methods
              // to "Virtual" methods in inheritance trees. That is - IF we want
              // to start preserving the offsets of virtuals. There is room in
              // the method struct to make this work, but for now I've just
              // decided to drop this information. It is not urgently useful to
              // us and greatly complicates matters.
              
              // extract vbaseoff
              U64 next_cursor = cursor + sizeof(*method);
              U32 vbaseoff = 0;
              if (prop == CV_MethodProp_Intro || prop == CV_MethodProp_PureIntro){
                if (cursor + sizeof(*method) + 4 <= cap){
                  vbaseoff = *(U32*)(method + 1);
                }
                next_cursor += 4;
              }
              
              // update cursor
              cursor = next_cursor;
              
              // TODO(allen): handle attribs
              
              // emit
              RADDBGIC_Type *mem_type = pdbconv_type_resolve_itype(ctx, method->itype);
              
              switch (prop){
                default:
                {
                  raddbgic_type_add_member_method(ctx->root, owner_type, name, mem_type);
                }break;
                
                case CV_MethodProp_Static:
                {
                  raddbgic_type_add_member_static_method(ctx->root, owner_type, name, mem_type);
                }break;
                
                case CV_MethodProp_Virtual:
                case CV_MethodProp_PureVirtual:
                case CV_MethodProp_Intro:
                case CV_MethodProp_PureIntro:
                {
                  raddbgic_type_add_member_virtual_method(ctx->root, owner_type, name, mem_type);
                }break;
              }
            }
            
          }
        }break;
        
        case CV_LeafKind_ONEMETHOD:
        {
          // TODO(allen): error if bad range
          if (list_item_off + sizeof(CV_LeafOneMethod) <= cap){
            // compute whole layout
            CV_LeafOneMethod *one_method = (CV_LeafOneMethod*)(first + list_item_off);
            
            CV_MethodProp prop = CV_FieldAttribs_ExtractMethodProp(one_method->attribs);
            
            U64 vbaseoff_off = list_item_off + sizeof(*one_method);
            U64 vbaseoff_opl_off = vbaseoff_off;
            U32 vbaseoff = 0;
            if (prop == CV_MethodProp_Intro || prop == CV_MethodProp_PureIntro){
              vbaseoff = *(U32*)(first + vbaseoff_off);
              vbaseoff_opl_off += sizeof(vbaseoff);
            }
            
            U64 name_off = vbaseoff_opl_off;
            String8 name = str8_cstring_capped(first + name_off, first + cap);
            
            list_item_opl_off = name_off + name.size + 1;
            
            // TODO(allen): handle attribs
            
            // emit
            RADDBGIC_Type *mem_type = pdbconv_type_resolve_itype(ctx, one_method->itype);
            
            switch (prop){
              default:
              {
                raddbgic_type_add_member_method(ctx->root, owner_type, name, mem_type);
              }break;
              
              case CV_MethodProp_Static:
              {
                raddbgic_type_add_member_static_method(ctx->root, owner_type, name, mem_type);
              }break;
              
              case CV_MethodProp_Virtual:
              case CV_MethodProp_PureVirtual:
              case CV_MethodProp_Intro:
              case CV_MethodProp_PureIntro:
              {
                raddbgic_type_add_member_virtual_method(ctx->root, owner_type, name, mem_type);
              }break;
            }
          }
        }break;
        
        case CV_LeafKind_NESTTYPE:
        {
          // TODO(allen): error if bad range
          if (list_item_off + sizeof(CV_LeafNestType) <= cap){
            // compute whole layout
            CV_LeafNestType *nest_type = (CV_LeafNestType*)(first + list_item_off);
            
            U64 name_off = list_item_off + sizeof(*nest_type);
            String8 name = str8_cstring_capped(first + name_off, first + cap);
            
            list_item_opl_off = name_off + name.size + 1;
            
            // emit member
            RADDBGIC_Type *mem_type = pdbconv_type_resolve_itype(ctx, nest_type->itype);
            raddbgic_type_add_member_nested_type(ctx->root, owner_type, mem_type);
          }
        }break;
        
        case CV_LeafKind_NESTTYPEEX:
        {
          // TODO(allen): error if bad range
          if (list_item_off + sizeof(CV_LeafNestTypeEx) <= cap){
            // compute whole layout
            CV_LeafNestTypeEx *nest_type = (CV_LeafNestTypeEx*)(first + list_item_off);
            
            U64 name_off = list_item_off + sizeof(*nest_type);
            String8 name = str8_cstring_capped(first + name_off, first + cap);
            
            list_item_opl_off = name_off + name.size + 1;
            
            // TODO(allen): handle attribs
            
            // emit member
            RADDBGIC_Type *mem_type = pdbconv_type_resolve_itype(ctx, nest_type->itype);
            raddbgic_type_add_member_nested_type(ctx->root, owner_type, mem_type);
          }
        }break;
        
        case CV_LeafKind_BCLASS:
        {
          // TODO(allen): error if bad range
          if (list_item_off + sizeof(CV_LeafBClass) <= cap){
            // compute whole layout
            CV_LeafBClass *bclass = (CV_LeafBClass*)(first + list_item_off);
            
            U64 offset_off = list_item_off + sizeof(*bclass);
            CV_NumericParsed offset = cv_numeric_from_data_range(first + offset_off, first + cap);
            
            list_item_opl_off = offset_off + offset.encoded_size;
            
            // TODO(allen): handle attribs
            
            // emit member
            RADDBGIC_Type *base_type = pdbconv_type_resolve_itype(ctx, bclass->itype);
            U32 offset_u32 = pdbconv_u32_from_numeric(ctx, &offset);
            raddbgic_type_add_member_base(ctx->root, owner_type, base_type, offset_u32);
          }
        }break;
        
        case CV_LeafKind_VBCLASS:
        case CV_LeafKind_IVBCLASS:
        {
          // TODO(allen): error if bad range
          if (list_item_off + sizeof(CV_LeafVBClass) <= cap){
            // compute whole layout
            CV_LeafVBClass *vbclass = (CV_LeafVBClass*)(first + list_item_off);
            
            U64 num1_off = list_item_off + sizeof(*vbclass);
            CV_NumericParsed num1 = cv_numeric_from_data_range(first + num1_off, first + cap);
            
            U64 num2_off = num1_off + num1.encoded_size;
            CV_NumericParsed num2 = cv_numeric_from_data_range(first + num2_off, first + cap);
            
            list_item_opl_off = num2_off + num2.encoded_size;
            
            // TODO(allen): handle attribs
            
            // emit member
            RADDBGIC_Type *base_type = pdbconv_type_resolve_itype(ctx, vbclass->itype);
            U32 vbptr_offset_u32  = pdbconv_u32_from_numeric(ctx, &num1);
            U32 vtable_offset_u32 = pdbconv_u32_from_numeric(ctx, &num2);
            raddbgic_type_add_member_virtual_base(ctx->root, owner_type, base_type,
                                                  vbptr_offset_u32, vtable_offset_u32);
          }
        }break;
        
        // discard cases - we don't currently do anything with these
        case CV_LeafKind_VFUNCTAB:
        {
          // TODO(rjf): error if bad range
          if(list_item_off + sizeof(CV_LeafVFuncTab) <= cap)
          {
            list_item_opl_off = list_item_off + sizeof(CV_LeafVFuncTab);
          }
        }break;
        
        // unhandled or invalid cases
        default:
        {
          String8 kind_str = cv_string_from_leaf_kind(field_kind);
          raddbgic_push_errorf(ctx->root, "unhandled/invalid case: equip_members -> %.*s",
                               str8_varg(kind_str));
        }break;
      }
      
      // update cursor
      U64 next_cursor = AlignPow2(list_item_opl_off, 4);
      cursor = next_cursor;
    }
  }
  
  scratch_end(scratch);
}

static void
pdbconv_type_equip_enumerates(PDBCONV_Ctx *ctx, RADDBGIC_Type *owner_type, CV_TypeId field_itype){
  Temp scratch = scratch_begin(0, 0);
  
  String8 data = ctx->leaf->data;
  
  // field stack
  // TODO(allen): add notes about field tasks
  struct FieldTask{
    struct FieldTask *next;
    CV_TypeId itype;
  };
  struct FieldTask *handled = 0;
  struct FieldTask *todo = 0;
  {
    struct FieldTask *task = push_array(scratch.arena, struct FieldTask, 1);
    SLLStackPush(todo, task);
    task->itype = field_itype;
  }
  
  for (;;){
    // exit condition
    if (todo == 0){
      break;
    }
    
    // determine itype
    CV_TypeId field_itype = todo->itype;
    {
      struct FieldTask *task = todo;
      SLLStackPop(todo);
      SLLStackPush(handled, task);
    }
    
    // get leaf range
    // TODO(allen): error if this itype is bad
    U8 *first = 0;
    U64 cap = 0;
    if (ctx->leaf->itype_first <= field_itype && field_itype < ctx->leaf->itype_opl){
      CV_RecRange *range = &ctx->leaf->leaf_ranges.ranges[field_itype - ctx->leaf->itype_first];
      // check valid arglist
      if (range->hdr.kind == CV_LeafKind_FIELDLIST &&
          range->off + range->hdr.size <= data.size){
        first = data.str + range->off + 2;
        cap = range->hdr.size - 2;
      }
    }
    
    U64 cursor = 0;
    for (;cursor + sizeof(CV_LeafKind) <= cap;){
      CV_LeafKind field_kind = *(CV_LeafKind*)(first + cursor);
      
      U64 list_item_off = cursor + 2;
      // if we hit an error or forget to set next cursor for a case
      // default to exiting the loop
      U64 list_item_opl_off = cap;
      
      switch (field_kind){
        case CV_LeafKind_INDEX:
        {
          // TODO(allen): error if bad range
          if (list_item_off + sizeof(CV_LeafIndex) <= cap){
            // compute whole layout
            CV_LeafIndex *index = (CV_LeafIndex*)(first + list_item_off);
            
            list_item_opl_off = list_item_off + sizeof(*index);
            
            // create new todo task
            CV_TypeId new_itype = index->itype;
            B32 is_new = 1;
            for (struct FieldTask *task = handled;
                 task != 0;
                 task = task->next){
              if (task->itype == new_itype){
                is_new = 0;
                break;
              }
            }
            if (is_new){
              struct FieldTask *task = push_array(scratch.arena, struct FieldTask, 1);
              SLLStackPush(todo, task);
              task->itype = new_itype;
            }
          }
        }break;
        
        case CV_LeafKind_ENUMERATE:
        {
          // compute whole layout
          CV_LeafEnumerate *enumerate = (CV_LeafEnumerate*)(first + list_item_off);
          
          U64 val_off = list_item_off + sizeof(*enumerate);
          CV_NumericParsed val = cv_numeric_from_data_range(first + val_off, first + cap);
          
          U64 name_off = val_off + val.encoded_size;
          String8 name = str8_cstring_capped(first + name_off, first + cap);
          
          list_item_opl_off = name_off + name.size + 1;
          
          // TODO(allen): handle attribs
          
          // emit enum val
          U64 val_u64 = cv_u64_from_numeric(&val);
          raddbgic_type_add_enum_val(ctx->root, owner_type, name, val_u64);
        }break;
        
        // unhandled or invalid cases
        default:
        {
          String8 kind_str = cv_string_from_leaf_kind(field_kind);
          raddbgic_push_errorf(ctx->root, "unhandled/invalid case: equip_enumerates -> %.*s",
                               str8_varg(kind_str));
        }break;
      }
      
      // update cursor
      U64 next_cursor = AlignPow2(list_item_opl_off, 4);
      cursor = next_cursor;
    }
  }
  
  scratch_end(scratch);
}

static RADDBGIC_Type*
pdbconv_type_cons_basic(PDBCONV_Ctx *ctx, CV_TypeId itype){
  Assert(itype < 0x1000);
  
  CV_BasicPointerKind basic_ptr_kind = CV_BasicPointerKindFromTypeId(itype);
  CV_BasicType basic_type_code = CV_BasicTypeFromTypeId(itype);
  
  RADDBGIC_Reservation *basic_res = raddbgic_type_reserve_id(ctx->root, basic_type_code, basic_type_code);
  
  RADDBGIC_Type *basic_type = 0;
  switch (basic_type_code){
    case CV_BasicType_VOID:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_Void, str8_lit("void"));
    }break;
    
    case CV_BasicType_HRESULT:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_Handle, str8_lit("HRESULT"));
    }break;
    
    case CV_BasicType_RCHAR:
    case CV_BasicType_CHAR:
    case CV_BasicType_CHAR8:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_Char8, str8_lit("char"));
    }break;
    
    case CV_BasicType_UCHAR:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_UChar8, str8_lit("UCHAR"));
    }break;
    
    case CV_BasicType_WCHAR:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_UChar16, str8_lit("WCHAR"));
    }break;
    
    case CV_BasicType_CHAR16:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_Char16, str8_lit("CHAR16"));
    }break;
    
    case CV_BasicType_CHAR32:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_Char32, str8_lit("CHAR32"));
    }break;
    
    case CV_BasicType_BOOL8:
    case CV_BasicType_INT8:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_S8, str8_lit("S8"));
    }break;
    
    case CV_BasicType_BOOL16:
    case CV_BasicType_INT16:
    case CV_BasicType_SHORT:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_S16, str8_lit("S16"));
    }break;
    
    case CV_BasicType_BOOL32:
    case CV_BasicType_INT32:
    case CV_BasicType_LONG:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_S32, str8_lit("S32"));
    }break;
    
    case CV_BasicType_BOOL64:
    case CV_BasicType_INT64:
    case CV_BasicType_QUAD:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_S64, str8_lit("S64"));
    }break;
    
    case CV_BasicType_INT128:
    case CV_BasicType_OCT:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_S128, str8_lit("S128"));
    }break;
    
    case CV_BasicType_UINT8:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_U8, str8_lit("U8"));
    }break;
    
    case CV_BasicType_UINT16:
    case CV_BasicType_USHORT:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_U16, str8_lit("U16"));
    }break;
    
    case CV_BasicType_UINT32:
    case CV_BasicType_ULONG:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_U32, str8_lit("U32"));
    }break;
    
    case CV_BasicType_UINT64:
    case CV_BasicType_UQUAD:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_U64, str8_lit("U64"));
    }break;
    
    case CV_BasicType_UINT128:
    case CV_BasicType_UOCT:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_U128, str8_lit("U128"));
    }break;
    
    case CV_BasicType_FLOAT16:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_F16, str8_lit("F16"));
    }break;
    
    case CV_BasicType_FLOAT32:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_F32, str8_lit("F32"));
    }break;
    
    case CV_BasicType_FLOAT32PP:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_F32PP, str8_lit("F32PP"));
    }break;
    
    case CV_BasicType_FLOAT48:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_F48, str8_lit("F48"));
    }break;
    
    case CV_BasicType_FLOAT64:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_F64, str8_lit("F64"));
    }break;
    
    case CV_BasicType_FLOAT80:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_F80, str8_lit("F80"));
    }break;
    
    case CV_BasicType_FLOAT128:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_F128, str8_lit("F128"));
    }break;
    
    case CV_BasicType_COMPLEX32:
    {
      basic_type =
        raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_ComplexF32, str8_lit("ComplexF32"));
    }break;
    
    case CV_BasicType_COMPLEX64:
    {
      basic_type =
        raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_ComplexF64, str8_lit("ComplexF64"));
    }break;
    
    case CV_BasicType_COMPLEX80:
    {
      basic_type =
        raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_ComplexF80, str8_lit("ComplexF80"));
    }break;
    
    case CV_BasicType_COMPLEX128:
    {
      basic_type =
        raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_ComplexF128, str8_lit("ComplexF128"));
    }break;
    
    case CV_BasicType_PTR:
    {
      basic_type = raddbgic_type_basic(ctx->root, RADDBGI_TypeKind_Handle, str8_lit("PTR"));
    }break;
  }
  
  // basic resolve
  raddbgic_type_fill_id(ctx->root, basic_res, basic_type);
  
  // wrap in constructed type
  RADDBGIC_Type *constructed_type = 0;
  if (basic_ptr_kind != 0 && basic_type != 0){
    RADDBGIC_Reservation *constructed_res = raddbgic_type_reserve_id(ctx->root, itype, itype);
    
    switch (basic_ptr_kind){
      case CV_BasicPointerKind_16BIT:
      case CV_BasicPointerKind_FAR_16BIT:
      case CV_BasicPointerKind_HUGE_16BIT:
      case CV_BasicPointerKind_32BIT:
      case CV_BasicPointerKind_16_32BIT:
      case CV_BasicPointerKind_64BIT:
      {
        constructed_type = raddbgic_type_pointer(ctx->root, basic_type, RADDBGI_TypeKind_Ptr);
      }break;
    }
    
    // constructed resolve
    raddbgic_type_fill_id(ctx->root, constructed_res, constructed_type);
  }
  
  // select output
  RADDBGIC_Type *result = basic_type;
  if (basic_ptr_kind != 0){
    result = constructed_type;
  }
  
  return(result);
}

static RADDBGIC_Type*
pdbconv_type_cons_leaf_record(PDBCONV_Ctx *ctx, CV_TypeId itype){
  Assert(ctx->leaf->itype_first <= itype && itype < ctx->leaf->itype_opl);
  
  RADDBGIC_Reservation *res = raddbgic_type_reserve_id(ctx->root, itype, itype);
  
  CV_RecRange *range = &ctx->leaf->leaf_ranges.ranges[itype - ctx->leaf->itype_first];
  String8 data = ctx->leaf->data;
  
  RADDBGIC_Type *result = 0;
  if (range->off + range->hdr.size <= data.size){
    U8 *first = data.str + range->off + 2;
    U64 cap = range->hdr.size - 2;
    
    switch (range->hdr.kind){
      case CV_LeafKind_MODIFIER:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafModifier) <= cap){
          CV_LeafModifier *modifier = (CV_LeafModifier*)first;
          
          RADDBGI_TypeModifierFlags flags = 0;
          if (modifier->flags & CV_ModifierFlag_Const){
            flags |= RADDBGI_TypeModifierFlag_Const;
          }
          if (modifier->flags & CV_ModifierFlag_Volatile){
            flags |= RADDBGI_TypeModifierFlag_Volatile;
          }
          
          RADDBGIC_Type *direct_type = pdbconv_type_resolve_and_check(ctx, modifier->itype);
          if (flags != 0){
            result = raddbgic_type_modifier(ctx->root, direct_type, flags);
          }
          else{
            result = direct_type;
          }
        }
      }break;
      
      case CV_LeafKind_POINTER:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafPointer) <= cap){
          CV_LeafPointer *pointer = (CV_LeafPointer*)first;
          
          CV_PointerKind ptr_kind = CV_PointerAttribs_ExtractKind(pointer->attribs);
          CV_PointerMode ptr_mode = CV_PointerAttribs_ExtractMode(pointer->attribs);
          U32 ptr_size            = CV_PointerAttribs_ExtractSize(pointer->attribs);
          
          // TODO(allen): if ptr_mode in {PtrMem, PtrMethod} then output a member pointer instead
          
          // extract modifier flags
          RADDBGI_TypeModifierFlags modifier_flags = 0;
          if (pointer->attribs & CV_PointerAttrib_Const){
            modifier_flags |= RADDBGI_TypeModifierFlag_Const;
          }
          if (pointer->attribs & CV_PointerAttrib_Volatile){
            modifier_flags |= RADDBGI_TypeModifierFlag_Volatile;
          }
          
          // determine type kind
          RADDBGI_TypeKind type_kind = RADDBGI_TypeKind_Ptr;
          if (pointer->attribs & CV_PointerAttrib_LRef){
            type_kind = RADDBGI_TypeKind_LRef;
          }
          else if (pointer->attribs & CV_PointerAttrib_RRef){
            type_kind = RADDBGI_TypeKind_RRef;
          }
          if (ptr_mode == CV_PointerMode_LRef){
            type_kind = RADDBGI_TypeKind_LRef;
          }
          else if (ptr_mode == CV_PointerMode_RRef){
            type_kind = RADDBGI_TypeKind_RRef;
          }
          
          RADDBGIC_Type *direct_type = pdbconv_type_resolve_and_check(ctx, pointer->itype);
          RADDBGIC_Type *ptr_type = raddbgic_type_pointer(ctx->root, direct_type, type_kind);
          
          result = ptr_type;
          if (modifier_flags != 0){
            result = raddbgic_type_modifier(ctx->root, ptr_type, modifier_flags);
          }
        }
      }break;
      
      case CV_LeafKind_PROCEDURE:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafProcedure) <= cap){
          CV_LeafProcedure *procedure = (CV_LeafProcedure*)first;
          
          Temp scratch = scratch_begin(0, 0);
          
          // TODO(allen): handle call_kind & attribs
          
          RADDBGIC_Type *ret_type = pdbconv_type_resolve_and_check(ctx, procedure->ret_itype);
          
          RADDBGIC_TypeList param_list = {0};
          pdbconv_type_resolve_arglist(scratch.arena, &param_list, ctx, procedure->arg_itype);
          
          result = raddbgic_type_proc(ctx->root, ret_type, &param_list);
          
          scratch_end(scratch);
        }
      }break;
      
      case CV_LeafKind_MFUNCTION:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafMFunction) <= cap){
          CV_LeafMFunction *mfunction = (CV_LeafMFunction*)first;
          
          Temp scratch = scratch_begin(0, 0);
          
          // TODO(allen): handle call_kind & attribs
          // TODO(allen): preserve "this_adjust"
          
          RADDBGIC_Type *ret_type = pdbconv_type_resolve_and_check(ctx, mfunction->ret_itype);
          
          RADDBGIC_TypeList param_list = {0};
          pdbconv_type_resolve_arglist(scratch.arena, &param_list, ctx, mfunction->arg_itype);
          
          RADDBGIC_Type *this_type = 0;
          if (mfunction->this_itype != 0){
            this_type = pdbconv_type_resolve_and_check(ctx, mfunction->this_itype);
            result = raddbgic_type_method(ctx->root, this_type, ret_type, &param_list);
          }
          else{
            result = raddbgic_type_proc(ctx->root, ret_type, &param_list);
          }
          
          scratch_end(scratch);
        }
      }break;
      
      case CV_LeafKind_BITFIELD:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafBitField) <= cap){
          CV_LeafBitField *bit_field = (CV_LeafBitField*)first;
          RADDBGIC_Type *direct_type = pdbconv_type_resolve_and_check(ctx, bit_field->itype);
          result = raddbgic_type_bitfield(ctx->root, direct_type, bit_field->pos, bit_field->len);
        }
      }break;
      
      case CV_LeafKind_ARRAY:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafArray) <= cap){
          CV_LeafArray *array = (CV_LeafArray*)first;
          
          // parse count
          U8 *numeric_ptr = (U8*)(array + 1);
          CV_NumericParsed array_count = cv_numeric_from_data_range(numeric_ptr, first + cap);
          
          U64 full_size = cv_u64_from_numeric(&array_count);
          
          RADDBGIC_Type *direct_type = pdbconv_type_resolve_and_check(ctx, array->entry_itype);
          U64 count = full_size;
          if (direct_type != 0 && direct_type->byte_size != 0){
            count /= direct_type->byte_size;
          }
          
          // build type
          result = raddbgic_type_array(ctx->root, direct_type, count);
        }
      }break;
      
      case CV_LeafKind_CLASS:
      case CV_LeafKind_STRUCTURE:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafStruct) <= cap){
          CV_LeafStruct *lf_struct = (CV_LeafStruct*)first;
          
          // TODO(allen): handle props
          
          // size
          U8 *numeric_ptr = (U8*)(lf_struct + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          U64 size_u64 = cv_u64_from_numeric(&size);
          
          // name
          U8 *name_ptr = numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // incomplete type
          if (lf_struct->props & CV_TypeProp_FwdRef){
            RADDBGI_TypeKind type_kind = RADDBGI_TypeKind_IncompleteStruct;
            if (range->hdr.kind == CV_LeafKind_CLASS){
              type_kind = RADDBGI_TypeKind_IncompleteClass;
            }
            result = raddbgic_type_incomplete(ctx->root, type_kind, name);
          }
          
          // complete type
          else{
            RADDBGI_TypeKind type_kind = RADDBGI_TypeKind_Struct;
            if (range->hdr.kind == CV_LeafKind_CLASS){
              type_kind = RADDBGI_TypeKind_Class;
            }
            result = raddbgic_type_udt(ctx->root, type_kind, name, size_u64);
            
            // remember to revisit this for members
            {
              PDBCONV_TypeRev *rev = push_array(ctx->arena, PDBCONV_TypeRev, 1);
              rev->owner_type = result;
              rev->field_itype = lf_struct->field_itype;
              SLLQueuePush(ctx->member_revisit_first, ctx->member_revisit_last, rev);
            }
          }
        }
      }break;
      
      case CV_LeafKind_CLASS2:
      case CV_LeafKind_STRUCT2:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafStruct2) <= cap){
          CV_LeafStruct2 *lf_struct = (CV_LeafStruct2*)first;
          
          // TODO(allen): handle props
          
          // size
          U8 *numeric_ptr = (U8*)(lf_struct + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          U64 size_u64 = cv_u64_from_numeric(&size);
          
          // name
          U8 *name_ptr = numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // incomplete type
          if (lf_struct->props & CV_TypeProp_FwdRef){
            RADDBGI_TypeKind type_kind = RADDBGI_TypeKind_IncompleteStruct;
            if (range->hdr.kind == CV_LeafKind_CLASS2){
              type_kind = RADDBGI_TypeKind_IncompleteClass;
            }
            result = raddbgic_type_incomplete(ctx->root, type_kind, name);
          }
          
          // complete type
          else{
            RADDBGI_TypeKind type_kind = RADDBGI_TypeKind_Struct;
            if (range->hdr.kind == CV_LeafKind_CLASS2){
              type_kind = RADDBGI_TypeKind_Class;
            }
            result = raddbgic_type_udt(ctx->root, type_kind, name, size_u64);
            
            // remember to revisit this for members
            {
              PDBCONV_TypeRev *rev = push_array(ctx->arena, PDBCONV_TypeRev, 1);
              rev->owner_type = result;
              rev->field_itype = lf_struct->field_itype;
              SLLQueuePush(ctx->member_revisit_first, ctx->member_revisit_last, rev);
            }
          }
        }
      }break;
      
      case CV_LeafKind_UNION:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafUnion) <= cap){
          CV_LeafUnion *lf_union = (CV_LeafUnion*)first;
          
          // TODO(allen): handle props
          
          // size
          U8 *numeric_ptr = (U8*)(lf_union + 1);
          CV_NumericParsed size = cv_numeric_from_data_range(numeric_ptr, first + cap);
          U64 size_u64 = cv_u64_from_numeric(&size);
          
          // name
          U8 *name_ptr = numeric_ptr + size.encoded_size;
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // incomplete type
          if (lf_union->props & CV_TypeProp_FwdRef){
            result =
              raddbgic_type_incomplete(ctx->root, RADDBGI_TypeKind_IncompleteUnion, name);
          }
          
          // complete type
          else{
            result = raddbgic_type_udt(ctx->root, RADDBGI_TypeKind_Union, name, size_u64);
            
            // remember to revisit this for members
            {
              PDBCONV_TypeRev *rev = push_array(ctx->arena, PDBCONV_TypeRev, 1);
              rev->owner_type = result;
              rev->field_itype = lf_union->field_itype;
              SLLQueuePush(ctx->member_revisit_first, ctx->member_revisit_last, rev);
            }
          }
        }
      }break;
      
      case CV_LeafKind_ENUM:
      {
        // TODO(allen): error if bad range
        if (sizeof(CV_LeafEnum) <= cap){
          CV_LeafEnum *lf_enum = (CV_LeafEnum*)first;
          
          // TODO(allen): handle props
          
          // name
          U8 *name_ptr = (U8*)(lf_enum + 1);
          String8 name = str8_cstring_capped((char*)name_ptr, first + cap);
          
          // incomplete type
          if (lf_enum->props & CV_TypeProp_FwdRef){
            result = raddbgic_type_incomplete(ctx->root, RADDBGI_TypeKind_IncompleteEnum, name);
          }
          
          // complete type
          else{
            RADDBGIC_Type *direct_type = pdbconv_type_resolve_and_check(ctx, lf_enum->base_itype);
            result = raddbgic_type_enum(ctx->root, direct_type, name);
            
            // remember to revisit this for enumerates
            {
              PDBCONV_TypeRev *rev = push_array(ctx->arena, PDBCONV_TypeRev, 1);
              rev->owner_type = result;
              rev->field_itype = lf_enum->field_itype;
              SLLQueuePush(ctx->enum_revisit_first, ctx->enum_revisit_last, rev);
            }
          }
        }
      }break;
      
      // discard cases - we currently discard these these intentionally
      //                 so we mark them as "handled nil"
      case CV_LeafKind_VTSHAPE:
      case CV_LeafKind_VFTABLE:
      case CV_LeafKind_LABEL:
      {
        result = raddbgic_type_handled_nil(ctx->root);
      }break;
      
      // do nothing cases - these get handled in special passes and
      //                    they should not be appearing as direct types
      //                    or parameter types for any other types
      //                    so if the input data is valid we won't get
      //                    error messages even if they resolve to nil
      case CV_LeafKind_FIELDLIST:
      case CV_LeafKind_ARGLIST:
      case CV_LeafKind_METHODLIST:
      {}break;
      
      // Leaf Kinds
      case CV_LeafKind_MODIFIER_16t:
      case CV_LeafKind_POINTER_16t:
      case CV_LeafKind_ARRAY_16t:
      case CV_LeafKind_CLASS_16t:
      case CV_LeafKind_STRUCTURE_16t:
      case CV_LeafKind_UNION_16t:
      case CV_LeafKind_ENUM_16t:
      case CV_LeafKind_PROCEDURE_16t:
      case CV_LeafKind_MFUNCTION_16t:
      //case CV_LeafKind_VTSHAPE:
      case CV_LeafKind_COBOL0_16t:
      case CV_LeafKind_COBOL1:
      case CV_LeafKind_BARRAY_16t:
      //case CV_LeafKind_LABEL:
      case CV_LeafKind_NULL:
      case CV_LeafKind_NOTTRAN:
      case CV_LeafKind_DIMARRAY_16t:
      case CV_LeafKind_VFTPATH_16t:
      case CV_LeafKind_PRECOMP_16t:
      case CV_LeafKind_ENDPRECOMP:
      case CV_LeafKind_OEM_16t:
      case CV_LeafKind_TYPESERVER_ST:
      case CV_LeafKind_SKIP_16t:
      case CV_LeafKind_ARGLIST_16t:
      case CV_LeafKind_DEFARG_16t:
      case CV_LeafKind_LIST:
      case CV_LeafKind_FIELDLIST_16t:
      case CV_LeafKind_DERIVED_16t:
      case CV_LeafKind_BITFIELD_16t:
      case CV_LeafKind_METHODLIST_16t:
      case CV_LeafKind_DIMCONU_16t:
      case CV_LeafKind_DIMCONLU_16t:
      case CV_LeafKind_DIMVARU_16t:
      case CV_LeafKind_DIMVARLU_16t:
      case CV_LeafKind_REFSYM:
      case CV_LeafKind_BCLASS_16t:
      case CV_LeafKind_VBCLASS_16t:
      case CV_LeafKind_IVBCLASS_16t:
      case CV_LeafKind_ENUMERATE_ST:
      case CV_LeafKind_FRIENDFCN_16t:
      case CV_LeafKind_INDEX_16t:
      case CV_LeafKind_MEMBER_16t:
      case CV_LeafKind_STMEMBER_16t:
      case CV_LeafKind_METHOD_16t:
      case CV_LeafKind_NESTTYPE_16t:
      case CV_LeafKind_VFUNCTAB_16t:
      case CV_LeafKind_FRIENDCLS_16t:
      case CV_LeafKind_ONEMETHOD_16t:
      case CV_LeafKind_VFUNCOFF_16t:
      case CV_LeafKind_TI16_MAX:
      //case CV_LeafKind_MODIFIER:
      //case CV_LeafKind_POINTER:
      case CV_LeafKind_ARRAY_ST:
      case CV_LeafKind_CLASS_ST:
      case CV_LeafKind_STRUCTURE_ST:
      case CV_LeafKind_UNION_ST:
      case CV_LeafKind_ENUM_ST:
      //case CV_LeafKind_PROCEDURE:
      //case CV_LeafKind_MFUNCTION:
      case CV_LeafKind_COBOL0:
      case CV_LeafKind_BARRAY:
      case CV_LeafKind_DIMARRAY_ST:
      case CV_LeafKind_VFTPATH:
      case CV_LeafKind_PRECOMP_ST:
      case CV_LeafKind_OEM:
      case CV_LeafKind_ALIAS_ST:
      case CV_LeafKind_OEM2:
      case CV_LeafKind_SKIP:
      //case CV_LeafKind_ARGLIST:
      case CV_LeafKind_DEFARG_ST:
      //case CV_LeafKind_FIELDLIST:
      case CV_LeafKind_DERIVED:
      //case CV_LeafKind_BITFIELD:
      //case CV_LeafKind_METHODLIST:
      case CV_LeafKind_DIMCONU:
      case CV_LeafKind_DIMCONLU:
      case CV_LeafKind_DIMVARU:
      case CV_LeafKind_DIMVARLU:
      case CV_LeafKind_BCLASS:
      case CV_LeafKind_VBCLASS:
      case CV_LeafKind_IVBCLASS:
      case CV_LeafKind_FRIENDFCN_ST:
      case CV_LeafKind_INDEX:
      case CV_LeafKind_MEMBER_ST:
      case CV_LeafKind_STMEMBER_ST:
      case CV_LeafKind_METHOD_ST:
      case CV_LeafKind_NESTTYPE_ST:
      case CV_LeafKind_VFUNCTAB:
      case CV_LeafKind_FRIENDCLS:
      case CV_LeafKind_ONEMETHOD_ST:
      case CV_LeafKind_VFUNCOFF:
      case CV_LeafKind_NESTTYPEEX_ST:
      case CV_LeafKind_MEMBERMODIFY_ST:
      case CV_LeafKind_MANAGED_ST:
      case CV_LeafKind_ST_MAX:
      case CV_LeafKind_TYPESERVER:
      case CV_LeafKind_ENUMERATE:
      //case CV_LeafKind_ARRAY:
      //case CV_LeafKind_CLASS:
      //case CV_LeafKind_STRUCTURE:
      //case CV_LeafKind_UNION:
      //case CV_LeafKind_ENUM:
      case CV_LeafKind_DIMARRAY:
      case CV_LeafKind_PRECOMP:
      case CV_LeafKind_ALIAS:
      case CV_LeafKind_DEFARG:
      case CV_LeafKind_FRIENDFCN:
      case CV_LeafKind_MEMBER:
      case CV_LeafKind_STMEMBER:
      case CV_LeafKind_METHOD:
      case CV_LeafKind_NESTTYPE:
      case CV_LeafKind_ONEMETHOD:
      case CV_LeafKind_NESTTYPEEX:
      case CV_LeafKind_MEMBERMODIFY:
      case CV_LeafKind_MANAGED:
      case CV_LeafKind_TYPESERVER2:
      case CV_LeafKind_STRIDED_ARRAY:
      case CV_LeafKind_HLSL:
      case CV_LeafKind_MODIFIER_EX:
      case CV_LeafKind_INTERFACE:
      case CV_LeafKind_BINTERFACE:
      case CV_LeafKind_VECTOR:
      case CV_LeafKind_MATRIX:
      //case CV_LeafKind_VFTABLE:
      default:
      {
        String8 kind_str = cv_string_from_leaf_kind(range->hdr.kind);
        raddbgic_push_errorf(ctx->root, "pdbconv: unhandled leaf case %.*s (0x%x)",
                             str8_varg(kind_str), range->hdr.kind);
      }break;
    }
  }
  
  raddbgic_type_fill_id(ctx->root, res, result);
  
  return(result);
}

static RADDBGIC_Type*
pdbconv_type_resolve_and_check(PDBCONV_Ctx *ctx, CV_TypeId itype){
  RADDBGIC_Type *result = pdbconv_type_resolve_itype(ctx, itype);
  if(raddbgic_type_is_unhandled_nil(ctx->root, result))
  {
    raddbgic_push_errorf(ctx->root, "pdbconv: could not resolve itype (itype = %u)", itype);
  }
  return(result);
}

static void
pdbconv_type_resolve_arglist(Arena *arena, RADDBGIC_TypeList *out,
                             PDBCONV_Ctx *ctx, CV_TypeId arglist_itype){
  ProfBeginFunction();
  
  // get leaf range
  if (ctx->leaf->itype_first <= arglist_itype && arglist_itype < ctx->leaf->itype_opl){
    CV_RecRange *range = &ctx->leaf->leaf_ranges.ranges[arglist_itype - ctx->leaf->itype_first];
    
    // check valid arglist
    String8 data = ctx->leaf->data;
    if (range->hdr.kind == CV_LeafKind_ARGLIST &&
        range->off + range->hdr.size <= data.size){
      U8 *first = data.str + range->off + 2;
      U64 cap = range->hdr.size - 2;
      if (sizeof(CV_LeafArgList) <= cap){
        
        // resolve parameters
        CV_LeafArgList *arglist = (CV_LeafArgList*)first;
        CV_TypeId *itypes = (CV_TypeId*)(arglist + 1);
        U32 max_count = (cap - sizeof(*arglist))/sizeof(CV_TypeId);
        U32 clamped_count = ClampTop(arglist->count, max_count);
        for (U32 i = 0; i < clamped_count; i += 1){
          RADDBGIC_Type *param_type = pdbconv_type_resolve_and_check(ctx, itypes[i]);
          raddbgic_type_list_push(arena,  out, param_type);
        }
        
      }
    }
  }
  
  ProfEnd();
}

static RADDBGIC_Type*
pdbconv_type_from_name(PDBCONV_Ctx *ctx, String8 name){
  // TODO(rjf): no idea if this is correct
  CV_TypeId cv_type_id = pdb_tpi_first_itype_from_name(ctx->hash, ctx->leaf, name, 0);
  RADDBGIC_Type *result = raddbgic_type_from_id(ctx->root, cv_type_id, cv_type_id);
  return(result);
}

static void
pdbconv_type_fwd_map_set(Arena *arena, PDBCONV_FwdMap *map, CV_TypeId key, CV_TypeId val){
  U64 bucket_idx = key%map->buckets_count;
  
  // search for an existing match
  PDBCONV_FwdNode *match = 0;
  for (PDBCONV_FwdNode *node = map->buckets[bucket_idx];
       node != 0;
       node = node->next){
    if (node->key == key){
      match = node;
      break;
    }
  }
  
  // create a new node if no match
  if (match == 0){
    match = push_array(arena, PDBCONV_FwdNode, 1);
    SLLStackPush(map->buckets[bucket_idx], match);
    match->key = key;
    map->pair_count += 1;
    map->bucket_collision_count += (match->next != 0);
  }
  
  // set node's val
  match->val = val;
}

static CV_TypeId
pdbconv_type_fwd_map_get(PDBCONV_FwdMap *map, CV_TypeId key){
  U64 bucket_idx = key%map->buckets_count;
  
  // search for an existing match
  PDBCONV_FwdNode *match = 0;
  for (PDBCONV_FwdNode *node = map->buckets[bucket_idx];
       node != 0;
       node = node->next){
    if (node->key == key){
      match = node;
      break;
    }
  }
  
  // extract result
  CV_TypeId result = 0;
  if (match != 0){
    result = match->val;
  }
  
  return(result);
}

//- symbols

static U64
pdbconv_hash_from_local_user_id(U64 sym_hash, U64 id)
{
  U64 hash = id ^ (sym_hash<<1) ^ (sym_hash<<4);
  return hash;
}

static U64
pdbconv_hash_from_scope_user_id(U64 sym_hash, U64 id)
{
  U64 hash = id ^ (sym_hash<<1) ^ (sym_hash<<4);
  return hash;
}

static U64
pdbconv_hash_from_symbol_user_id(U64 sym_hash, U64 id)
{
  U64 hash = id/8 + id ^ (sym_hash<<1) ^ (sym_hash<<4);
  return hash;
}

static void
pdbconv_symbol_cons(PDBCONV_Ctx *ctx, CV_SymParsed *sym, U32 sym_unique_id){
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  // extract important values from parameters
  String8 data = sym->data;
  U64 user_id_base = (((U64)sym_unique_id) << 32);
  U64 sym_unique_id_hash = raddbgi_hash((U8*)&sym_unique_id, sizeof(sym_unique_id));
  
  // PASS 1: map out data associations
  ProfScope("map out data associations")
  {
    // state variables
    RADDBGIC_Symbol *current_proc = 0;
    
    // loop
    CV_RecRange *rec_range = sym->sym_ranges.ranges;
    CV_RecRange *opl = rec_range + sym->sym_ranges.count;
    for(;rec_range < opl; rec_range += 1)
    {
      // symbol data range
      U64 opl_off_raw = rec_range->off + rec_range->hdr.size;
      U64 opl_off = ClampTop(opl_off_raw, data.size);
      
      U64 off_raw = rec_range->off + 2;
      U64 off = ClampTop(off_raw, opl_off);
      
      U8 *first = data.str + off;
      U64 cap = (opl_off - off);
      
      CV_SymKind kind = rec_range->hdr.kind; 
      switch(kind)
      {
        default:{}break;
        
        case CV_SymKind_FRAMEPROC:
        {
          if (sizeof(CV_SymFrameproc) > cap){
            // TODO(allen): error
          }
          else{
            CV_SymFrameproc *frameproc = (CV_SymFrameproc*)first;
            if (current_proc == 0){
              // TODO(allen): error
            }
            else{
              PDBCONV_FrameProcData data = {0};
              data.frame_size = frameproc->frame_size;
              data.flags = frameproc->flags;
              pdbconv_symbol_frame_proc_write(ctx, current_proc, &data);
            }
          }
        }break;
        
        case CV_SymKind_LPROC32:
        case CV_SymKind_GPROC32:
        {
          U64 symbol_id = user_id_base + off;
          U64 symbol_hash = pdbconv_hash_from_symbol_user_id(sym_unique_id_hash, symbol_id);
          current_proc = raddbgic_symbol_handle_from_user_id(ctx->root, symbol_id, symbol_hash);
        }break;
      }
    }
  }
  
  // PASS 2: main symbol construction pass
  ProfScope("main symbol construction pass")
  {
    // state variables
    RADDBGIC_LocationSet *defrange_target = 0;
    B32 defrange_target_is_param = 0;
    
    // loop
    CV_RecRange *rec_range = sym->sym_ranges.ranges;
    CV_RecRange *opl = rec_range + sym->sym_ranges.count;
    U64 local_num = 1;
    U64 scope_num = 1;
    for(;rec_range < opl; rec_range += 1)
    {
      // symbol data range
      U64 opl_off_raw = rec_range->off + rec_range->hdr.size;
      U64 opl_off = ClampTop(opl_off_raw, data.size);
      
      U64 off_raw = rec_range->off + 2;
      U64 off = ClampTop(off_raw, opl_off);
      
      U8 *first = data.str + off;
      U64 cap = (opl_off - off);
      
      // current state
      RADDBGIC_Scope *current_scope = pdbconv_symbol_current_scope(ctx);
      RADDBGIC_Symbol *current_procedure = 0;
      if(current_scope != 0)
      {
        current_procedure = current_scope->symbol;
      }
      
      CV_SymKind kind = rec_range->hdr.kind; 
      switch(kind)
      {
        default:{}break;
        
        case CV_SymKind_END:
        //ProfScope("CV_SymKind_END")
        {
          // pop scope stack
          pdbconv_symbol_pop_scope(ctx);
          defrange_target = 0;
          defrange_target_is_param = 0;
        }break;
        
        case CV_SymKind_FRAMEPROC:
        //ProfScope("CV_SymKind_FRAMEPROC")
        {
          if (sizeof(CV_SymFrameproc) > cap){
            // TODO(allen): error
          }
          else{
            // do nothing (handled in 'association map' pass)
          }
        }break;
        
        case CV_SymKind_BLOCK32:
        //ProfScope("CV_SymKind_BLOCK32")
        {
          if (sizeof(CV_SymBlock32) > cap){
            // TODO(allen): error
          }
          else{
            CV_SymBlock32 *block32 = (CV_SymBlock32*)first;
            
            // scope
            U64 scope_id = user_id_base + scope_num;
            U64 scope_hash = pdbconv_hash_from_scope_user_id(sym_unique_id_hash, scope_id);
            scope_num += 1;
            RADDBGIC_Scope *block_scope = raddbgic_scope_handle_from_user_id(ctx->root, scope_id, scope_hash);
            raddbgic_scope_set_parent(ctx->root, block_scope, current_scope);
            pdbconv_symbol_push_scope(ctx, block_scope, current_procedure);
            
            // set voff range
            COFF_SectionHeader *section = pdbconv_sec_header_from_sec_num(ctx, block32->sec);
            if (section != 0){
              U64 voff_first = section->voff + block32->off;
              U64 voff_last = voff_first + block32->len;
              raddbgic_scope_add_voff_range(ctx->root, block_scope, voff_first, voff_last);
            }
          }
        }break;
        
        case CV_SymKind_LDATA32:
        case CV_SymKind_GDATA32:
        //ProfScope("CV_SymKind_LDATA32/CV_SymKind_GDATA32")
        {
          if (sizeof(CV_SymData32) > cap){
            // TODO(allen): error
          }
          else{
            CV_SymData32 *data32 = (CV_SymData32*)first;
            
            // name
            String8 name = str8_cstring_capped((char*)(data32 + 1), first + cap);
            
            // determine voff
            COFF_SectionHeader *section = pdbconv_sec_header_from_sec_num(ctx, data32->sec);
            U64 voff = ((section != 0)?section->voff:0) + data32->off;
            
            // deduplicate global variable symbols with the same name & offset
            // * PDB likes to have duplicates of these spread across
            // * different symbol streams so we deduplicate across the
            // * entire translation context.
            if (!pdbconv_known_global_lookup(&ctx->known_globals, name, voff)){
              pdbconv_known_global_insert(ctx->arena, &ctx->known_globals, name, voff);
              
              // type of variable
              RADDBGIC_Type *type = pdbconv_type_resolve_itype(ctx, data32->itype);
              
              // container type
              RADDBGIC_Type *container_type = 0;
              U64 container_name_opl = pdbconv_end_of_cplusplus_container_name(name);
              if (container_name_opl > 2){
                String8 container_name = str8(name.str, container_name_opl - 2);
                container_type = pdbconv_type_from_name(ctx, container_name);
              }
              
              // container symbol
              RADDBGIC_Symbol *container_symbol = 0;
              if (container_type == 0){
                container_symbol = current_procedure;
              }
              
              // determine link kind
              B32 is_extern = (kind == CV_SymKind_GDATA32);
              
              // cons this symbol
              U64 symbol_id = user_id_base + off;
              U64 symbol_hash = pdbconv_hash_from_symbol_user_id(sym_unique_id_hash, symbol_id);
              RADDBGIC_Symbol *symbol = raddbgic_symbol_handle_from_user_id(ctx->root, symbol_id, symbol_hash);
              
              RADDBGIC_SymbolInfo info = zero_struct;
              info.kind = RADDBGIC_SymbolKind_GlobalVariable;
              info.name = name;
              info.type = type;
              info.is_extern = is_extern;
              info.offset = voff;
              info.container_type = container_type;
              info.container_symbol = container_symbol;
              
              raddbgic_symbol_set_info(ctx->root, symbol, &info);
            }
          }
        }break;
        
        case CV_SymKind_LPROC32:
        case CV_SymKind_GPROC32:
        //ProfScope("CV_SymKind_LPROC32/CV_SymKind_GPROC32")
        {
          if (sizeof(CV_SymProc32) > cap){
            // TODO(allen): error
          }
          else{
            CV_SymProc32 *proc32 = (CV_SymProc32*)first;
            
            // name
            String8 name = str8_cstring_capped((char*)(proc32 + 1), first + cap);
            
            // type of procedure
            RADDBGIC_Type *type = pdbconv_type_resolve_itype(ctx, proc32->itype);
            
            // container type
            RADDBGIC_Type *container_type = 0;
            U64 container_name_opl = pdbconv_end_of_cplusplus_container_name(name);
            if (container_name_opl > 2){
              String8 container_name = str8(name.str, container_name_opl - 2);
              container_type = pdbconv_type_from_name(ctx, container_name);
            }
            
            // container symbol
            RADDBGIC_Symbol *container_symbol = 0;
            if (container_type == 0){
              container_symbol = current_procedure;
            }
            
            // get this symbol handle
            U64 symbol_id = user_id_base + off;
            U64 symbol_hash = pdbconv_hash_from_symbol_user_id(sym_unique_id_hash, symbol_id);
            RADDBGIC_Symbol *proc_symbol = raddbgic_symbol_handle_from_user_id(ctx->root, symbol_id, symbol_hash);
            
            // scope
            
            // NOTE: even if there could be a containing scope at this point (which should be
            //       illegal in C/C++ but not necessarily in another language) we would not pass
            //       it here because these scopes refer to the ranges of code that make up a
            //       procedure *not* the namespaces, so a procedure's root scope always has
            //       no parent.
            U64 scope_id = user_id_base + scope_num;
            U64 scope_hash = pdbconv_hash_from_scope_user_id(sym_unique_id_hash, scope_id);
            RADDBGIC_Scope *root_scope = raddbgic_scope_handle_from_user_id(ctx->root, scope_id, scope_hash);
            pdbconv_symbol_push_scope(ctx, root_scope, proc_symbol);
            scope_num += 1;
            
            // set voff range
            U64 voff = 0;
            COFF_SectionHeader *section = pdbconv_sec_header_from_sec_num(ctx, proc32->sec);
            if (section != 0){
              U64 voff_first = section->voff + proc32->off;
              U64 voff_last = voff_first + proc32->len;
              raddbgic_scope_add_voff_range(ctx->root, root_scope, voff_first, voff_last);
              
              voff = voff_first;
            }
            
            // link name
            String8 link_name = {0};
            if (voff != 0){
              link_name = pdbconv_link_name_find(&ctx->link_names, voff);
            }
            
            // determine link kind
            B32 is_extern = (kind == CV_SymKind_GPROC32);
            
            // set symbol info
            RADDBGIC_SymbolInfo info = zero_struct;
            info.kind = RADDBGIC_SymbolKind_Procedure;
            info.name = name;
            info.link_name = link_name;
            info.type = type;
            info.is_extern = is_extern;
            info.container_type = container_type;
            info.container_symbol = container_symbol;
            info.root_scope = root_scope;
            
            raddbgic_symbol_set_info(ctx->root, proc_symbol, &info);
          }
        }break;
        
        case CV_SymKind_REGREL32:
        ProfScope("CV_SymKind_REGREL32")
        {
          if (sizeof(CV_SymRegrel32) > cap){
            // TODO(allen): error
          }
          else{
            // TODO(allen): hide this when it's redundant with better information
            // from a CV_SymKind_LOCAL record.
            
            CV_SymRegrel32 *regrel32 = (CV_SymRegrel32*)first;
            
            // name
            String8 name = str8_cstring_capped((char*)(regrel32 + 1), first + cap);
            
            // type of variable
            RADDBGIC_Type *type = pdbconv_type_resolve_itype(ctx, regrel32->itype);
            
            // extract regrel's info
            CV_Reg cv_reg = regrel32->reg;
            U32 var_off = regrel32->reg_off;
            
            // need arch for analyzing register stuff
            RADDBGI_Arch arch = ctx->arch;
            U64 addr_size = ctx->addr_size;
            
            // determine if this is a parameter
            RADDBGI_LocalKind local_kind = RADDBGI_LocalKind_Variable;
            {
              B32 is_stack_reg = 0;
              switch (arch){
                case RADDBGI_Arch_X86: is_stack_reg = (cv_reg == CV_Regx86_ESP); break;
                case RADDBGI_Arch_X64: is_stack_reg = (cv_reg == CV_Regx64_RSP); break;
              }
              if (is_stack_reg){
                U32 frame_size = 0xFFFFFFFF;
                if (current_procedure != 0){
                  PDBCONV_FrameProcData *frameproc =
                    pdbconv_symbol_frame_proc_read(ctx, current_procedure);
                  frame_size = frameproc->frame_size;
                }
                if (var_off > frame_size){
                  local_kind = RADDBGI_LocalKind_Parameter;
                }
              }
            }
            
            // emit local
            U64 local_id = user_id_base + local_num;;
            U64 local_id_hash = pdbconv_hash_from_local_user_id(sym_unique_id_hash, local_id);
            RADDBGIC_Local *local_var = raddbgic_local_handle_from_user_id(ctx->root, local_id, local_id_hash);
            local_num += 1;
            
            RADDBGIC_LocalInfo info = {0};
            info.kind = local_kind;
            info.scope = current_scope;
            info.name = name;
            info.type = type;
            raddbgic_local_set_basic_info(ctx->root, local_var, &info);
            
            // add location to local
            {
              // will there be an extra indirection to the value
              B32 extra_indirection_to_value = 0;
              switch (arch){
                case RADDBGI_Arch_X86:
                {
                  if (local_kind == RADDBGI_LocalKind_Parameter &&
                      (type->byte_size > 4 || !IsPow2OrZero(type->byte_size))){
                    extra_indirection_to_value = 1;
                  }
                }break;
                
                case RADDBGI_Arch_X64:
                {
                  if (local_kind == RADDBGI_LocalKind_Parameter &&
                      (type->byte_size > 8 || !IsPow2OrZero(type->byte_size))){
                    extra_indirection_to_value = 1;
                  }
                }break;
              }
              
              // get raddbg register code
              RADDBGI_RegisterCode register_code = raddbgi_reg_code_from_cv_reg_code(arch, cv_reg);
              // TODO(allen): real byte_size & byte_pos from cv_reg goes here
              U32 byte_size = 8;
              U32 byte_pos = 0;
              
              // set location case
              RADDBGIC_Location *loc =
                pdbconv_location_from_addr_reg_off(ctx, register_code, byte_size, byte_pos,
                                                   (S64)(S32)var_off, extra_indirection_to_value);
              
              RADDBGIC_LocationSet *locset = raddbgic_location_set_from_local(ctx->root, local_var);
              raddbgic_location_set_add_case(ctx->root, locset, 0, max_U64, loc);
            }
          }
        }break;
        
        case CV_SymKind_LTHREAD32:
        case CV_SymKind_GTHREAD32:
        //ProfScope("CV_SymKind_LTHREAD32/CV_SymKind_GTHREAD32")
        {
          if (sizeof(CV_SymThread32) > cap){
            // TODO(allen): error
          }
          else{
            CV_SymThread32 *thread32 = (CV_SymThread32*)first;
            
            // name
            String8 name = str8_cstring_capped((char*)(thread32 + 1), first + cap);
            
            // determine tls off
            U32 tls_off = thread32->tls_off;
            
            // type of variable
            RADDBGIC_Type *type = pdbconv_type_resolve_itype(ctx, thread32->itype);
            
            // container type
            RADDBGIC_Type *container_type = 0;
            U64 container_name_opl = pdbconv_end_of_cplusplus_container_name(name);
            if (container_name_opl > 2){
              String8 container_name = str8(name.str, container_name_opl - 2);
              container_type = pdbconv_type_from_name(ctx, container_name);
            }
            
            // container symbol
            RADDBGIC_Symbol *container_symbol = 0;
            if (container_type == 0){
              container_symbol = current_procedure;
            }
            
            // determine link kind
            B32 is_extern = (kind == CV_SymKind_GTHREAD32);
            
            // setup symbol
            U64 symbol_id = user_id_base + off;
            U64 symbol_hash = pdbconv_hash_from_symbol_user_id(sym_unique_id_hash, symbol_id);
            RADDBGIC_Symbol *symbol = raddbgic_symbol_handle_from_user_id(ctx->root, symbol_id, symbol_hash);
            
            RADDBGIC_SymbolInfo info = zero_struct;
            info.kind = RADDBGIC_SymbolKind_ThreadVariable;
            info.name = name;
            info.type = type;
            info.is_extern = is_extern;
            info.offset = tls_off;
            info.container_type = container_type;
            info.container_symbol = container_symbol;
            
            raddbgic_symbol_set_info(ctx->root, symbol, &info);
          }
        }break;
        
        case CV_SymKind_LOCAL:
        //ProfScope("CV_SymKind_LOCAL")
        {
          if (sizeof(CV_SymLocal) > cap){
            // TODO(allen): error
          }
          else{
            CV_SymLocal *slocal = (CV_SymLocal*)first;
            
            // name
            String8 name = str8_cstring_capped((char*)(slocal + 1), first + cap);
            
            // type of variable
            RADDBGIC_Type *type = pdbconv_type_resolve_itype(ctx, slocal->itype);
            
            // determine how to handle
            B32 begin_a_global_modification = 0;
            if ((slocal->flags & CV_LocalFlag_Global) ||
                (slocal->flags & CV_LocalFlag_Static)){
              begin_a_global_modification = 1;
            }
            
            // emit a global modification
            if (begin_a_global_modification){
              // TODO(allen): add global modification symbols
              defrange_target = 0;
              defrange_target_is_param = 0;
            }
            
            // emit a local variable
            else{
              // local kind
              RADDBGI_LocalKind local_kind = RADDBGI_LocalKind_Variable;
              if (slocal->flags & CV_LocalFlag_Param){
                local_kind = RADDBGI_LocalKind_Parameter;
              }
              
              // emit local
              U64 local_id = user_id_base + local_num;
              U64 local_id_hash = pdbconv_hash_from_local_user_id(sym_unique_id_hash, local_id);
              RADDBGIC_Local *local_var = raddbgic_local_handle_from_user_id(ctx->root, local_id, local_id_hash);
              local_num += 1;
              local_var->kind = local_kind;
              local_var->name = name;
              local_var->type = type;
              
              RADDBGIC_LocalInfo info = {0};
              info.kind = local_kind;
              info.scope = current_scope;
              info.name = name;
              info.type = type;
              raddbgic_local_set_basic_info(ctx->root, local_var, &info);
              
              defrange_target = raddbgic_location_set_from_local(ctx->root, local_var);
              defrange_target_is_param = (local_kind == RADDBGI_LocalKind_Parameter);
            }
          }
        }break;
        
        case CV_SymKind_DEFRANGE_REGISTER:
        //ProfScope("CV_SymKind_DEFRANGE_REGISTER")
        {
          if (sizeof(CV_SymDefrangeRegister) > cap){
            // TODO(allen): error
          }
          else{
            if (defrange_target == 0){
              // TODO(allen): error
            }
            else{
              CV_SymDefrangeRegister *defrange_register = (CV_SymDefrangeRegister*)first;
              
              // TODO(allen): offset & size from cv_reg code
              RADDBGI_Arch arch = ctx->arch;
              CV_Reg cv_reg = defrange_register->reg;
              RADDBGI_RegisterCode register_code = raddbgi_reg_code_from_cv_reg_code(arch, cv_reg);
              
              // setup location
              RADDBGIC_Location *location = raddbgic_location_val_reg(ctx->root, register_code);
              
              // extract range info
              CV_LvarAddrRange *range = &defrange_register->range;
              CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_register + 1);
              U64 gap_count = ((first + cap) - (U8*)gaps)/sizeof(*gaps);
              
              // emit locations
              pdbconv_location_over_lvar_addr_range(ctx, defrange_target, location,
                                                    range, gaps, gap_count);
            }
          }
        }break;
        
        case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL:
        //ProfScope("CV_SymKind_DEFRANGE_FRAMEPOINTER_REL")
        {
          if (sizeof(CV_SymDefrangeFramepointerRel) > cap){
            // TODO(allen): error
          }
          else{
            if (defrange_target == 0){
              // TODO(allen): error
            }
            else{
              CV_SymDefrangeFramepointerRel *defrange_fprel = (CV_SymDefrangeFramepointerRel*)first;
              
              // select frame pointer register
              CV_EncodedFramePtrReg encoded_fp_reg =
                pdbconv_cv_encoded_fp_reg_from_proc(ctx, current_procedure, defrange_target_is_param);
              RADDBGI_RegisterCode fp_register_code =
                pdbconv_reg_code_from_arch_encoded_fp_reg(ctx->arch, encoded_fp_reg);
              
              // setup location
              B32 extra_indirection = 0;
              U32 byte_size = ctx->addr_size;
              U32 byte_pos = 0;
              S64 var_off = (S64)defrange_fprel->off;
              RADDBGIC_Location *location =
                pdbconv_location_from_addr_reg_off(ctx, fp_register_code, byte_size, byte_pos,
                                                   var_off, extra_indirection);
              
              // extract range info
              CV_LvarAddrRange *range = &defrange_fprel->range;
              CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_fprel + 1);
              U64 gap_count = ((first + cap) - (U8*)gaps)/sizeof(*gaps);
              
              // emit locations
              pdbconv_location_over_lvar_addr_range(ctx, defrange_target, location,
                                                    range, gaps, gap_count);
            }
          }
        }break;
        
        case CV_SymKind_DEFRANGE_SUBFIELD_REGISTER:
        //ProfScope("CV_SymKind_DEFRANGE_SUBFIELD_REGISTER")
        {
          if (sizeof(CV_SymDefrangeSubfieldRegister) > cap){
            // TODO(allen): error
          }
          else{
            if (defrange_target == 0){
              // TODO(allen): error
            }
            else{
              CV_SymDefrangeSubfieldRegister *defrange_subfield_register = (CV_SymDefrangeSubfieldRegister*)first;
              
              // TODO(allen): full "subfield" location system
              if (defrange_subfield_register->field_offset == 0){
                
                // TODO(allen): offset & size from cv_reg code
                RADDBGI_Arch arch = ctx->arch;
                CV_Reg cv_reg = defrange_subfield_register->reg;
                RADDBGI_RegisterCode register_code = raddbgi_reg_code_from_cv_reg_code(arch, cv_reg);
                
                // setup location
                RADDBGIC_Location *location = raddbgic_location_val_reg(ctx->root, register_code);
                
                // extract range info
                CV_LvarAddrRange *range = &defrange_subfield_register->range;
                CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_subfield_register + 1);
                U64 gap_count = ((first + cap) - (U8*)gaps)/sizeof(*gaps);
                
                // emit locations
                pdbconv_location_over_lvar_addr_range(ctx, defrange_target, location,
                                                      range, gaps, gap_count);
              }
            }
          }
        }break;
        
        case CV_SymKind_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE:
        //ProfScope("CV_SymKind_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE")
        {
          if (sizeof(CV_SymDefrangeFramepointerRelFullScope) > cap){
            // TODO(allen): error
          }
          else{
            if (defrange_target == 0){
              // TODO(allen): error
            }
            else{
              CV_SymDefrangeFramepointerRelFullScope *defrange_fprel_full_scope =
              (CV_SymDefrangeFramepointerRelFullScope*)first;
              
              // select frame pointer register
              CV_EncodedFramePtrReg encoded_fp_reg =
                pdbconv_cv_encoded_fp_reg_from_proc(ctx, current_procedure, defrange_target_is_param);
              RADDBGI_RegisterCode fp_register_code =
                pdbconv_reg_code_from_arch_encoded_fp_reg(ctx->arch, encoded_fp_reg);
              
              // setup location
              B32 extra_indirection = 0;
              U32 byte_size = ctx->addr_size;
              U32 byte_pos = 0;
              S64 var_off = (S64)defrange_fprel_full_scope->off;
              RADDBGIC_Location *location =
                pdbconv_location_from_addr_reg_off(ctx, fp_register_code, byte_size, byte_pos,
                                                   var_off, extra_indirection);
              
              
              // emit location
              raddbgic_location_set_add_case(ctx->root, defrange_target, 0, max_U64, location);
            }
          }
        }break;
        
        case CV_SymKind_DEFRANGE_REGISTER_REL:
        //ProfScope("CV_SymKind_DEFRANGE_REGISTER_REL")
        {
          if (sizeof(CV_SymDefrangeRegisterRel) > cap){
            // TODO(allen): error
          }
          else{
            CV_SymDefrangeRegisterRel *defrange_register_rel = (CV_SymDefrangeRegisterRel*)first;
            if(defrange_target == 0)
            {
              // TODO(rjf): error
            }
            else
            {
              // TODO(allen): offset & size from cv_reg code
              RADDBGI_Arch arch = ctx->arch;
              CV_Reg cv_reg = defrange_register_rel->reg;
              RADDBGI_RegisterCode register_code = raddbgi_reg_code_from_cv_reg_code(arch, cv_reg);
              U32 byte_size = ctx->addr_size;
              U32 byte_pos = 0;
              
              B32 extra_indirection_to_value = 0;
              S64 var_off = defrange_register_rel->reg_off;
              
              // setup location
              RADDBGIC_Location *location =
                pdbconv_location_from_addr_reg_off(ctx, register_code, byte_size, byte_pos,
                                                   var_off, extra_indirection_to_value);
              
              // extract range info
              CV_LvarAddrRange *range = &defrange_register_rel->range;
              CV_LvarAddrGap *gaps = (CV_LvarAddrGap*)(defrange_register_rel + 1);
              U64 gap_count = ((first + cap) - (U8*)gaps)/sizeof(*gaps);
              
              
              // emit locations
              pdbconv_location_over_lvar_addr_range(ctx, defrange_target, location,
                                                    range, gaps, gap_count);
            }
          }
        }break;
        
        case CV_SymKind_FILESTATIC:
        //ProfScope("CV_SymKind_FILESTATIC")
        {
          if (sizeof(CV_SymFileStatic) > cap){
            // TODO(allen): error
          }
          else{
            CV_SymFileStatic *file_static = (CV_SymFileStatic*)first;
            
            // name
            String8 name = str8_cstring_capped((char*)(file_static + 1), first + cap);
            
            // type of variable
            RADDBGIC_Type *type = pdbconv_type_resolve_itype(ctx, file_static->itype);
            
            // TODO(allen): emit a global modifier symbol
            
            // defrange records from this point attach to this location information
            defrange_target = 0;
            defrange_target_is_param = 0;
          }
        }break;
      }
    }
    
    // if scope stack isn't empty emit an error
    {
      RADDBGIC_Scope* scope = pdbconv_symbol_current_scope(ctx);
      if(scope != 0)
      {
        // TODO(allen): emit error
      }
    }
    
    // clear the scope stack
    pdbconv_symbol_clear_scope_stack(ctx);
  }
  
  scratch_end(scratch);
  ProfEnd();
}

static void
pdbconv_gather_link_names(PDBCONV_Ctx *ctx, CV_SymParsed *sym){
  ProfBeginFunction();
  // extract important values from parameters
  String8 data = sym->data;
  
  // loop
  CV_RecRange *rec_range = sym->sym_ranges.ranges;
  CV_RecRange *opl = rec_range + sym->sym_ranges.count;
  for (;rec_range < opl; rec_range += 1){
    // symbol data range
    U64 opl_off_raw = rec_range->off + rec_range->hdr.size;
    U64 opl_off = ClampTop(opl_off_raw, data.size);
    
    U64 off_raw = rec_range->off + 2;
    U64 off = ClampTop(off_raw, opl_off);
    
    U8 *first = data.str + off;
    U64 cap = (opl_off - off);
    
    CV_SymKind kind = rec_range->hdr.kind; 
    switch (kind){
      default: break;
      
      case CV_SymKind_PUB32:
      {
        if (sizeof(CV_SymPub32) > cap){
          // TODO(allen): error
        }
        else{
          CV_SymPub32 *pub32 = (CV_SymPub32*)first;
          
          // name
          String8 name = str8_cstring_capped((char*)(pub32 + 1), first + cap);
          
          // calculate voff
          U64 voff = 0;
          COFF_SectionHeader *section = pdbconv_sec_header_from_sec_num(ctx, pub32->sec);
          if (section != 0){
            voff = section->voff + pub32->off;
          }
          
          // save link name
          pdbconv_link_name_save(ctx->arena, &ctx->link_names, voff, name);
        }
      }break;
    }
  }
  ProfEnd();
}

// "frameproc" map

static void
pdbconv_symbol_frame_proc_write(PDBCONV_Ctx *ctx,RADDBGIC_Symbol *key,PDBCONV_FrameProcData *data){
  ProfBeginFunction();
  U64 key_int = IntFromPtr(key);
  PDBCONV_FrameProcMap *map = &ctx->frame_proc_map;
  U32 bucket_idx = key_int%map->buckets_count;
  
  // find match
  PDBCONV_FrameProcNode *match = 0;
  for (PDBCONV_FrameProcNode *node = map->buckets[bucket_idx];
       node != 0;
       node = node->next){
    if (node->key == key){
      match = node;
      break;
    }
  }
  
  // if there is already a match emit error
  if (match != 0){
    // TODO(allen): error
  }
  
  // insert new association if no match
  if (match == 0){
    match = push_array(ctx->arena, PDBCONV_FrameProcNode, 1);
    SLLStackPush(map->buckets[bucket_idx], match);
    match->key = key;
    MemoryCopyStruct(&match->data, data);
    map->pair_count += 1;
    map->bucket_collision_count += (match->next != 0);
  }
  ProfEnd();
}

static PDBCONV_FrameProcData*
pdbconv_symbol_frame_proc_read(PDBCONV_Ctx *ctx, RADDBGIC_Symbol *key){
  U64 key_int = IntFromPtr(key);
  PDBCONV_FrameProcMap *map = &ctx->frame_proc_map;
  U32 bucket_idx = key_int%map->buckets_count;
  
  // find match
  PDBCONV_FrameProcData *result = 0;
  for (PDBCONV_FrameProcNode *node = map->buckets[bucket_idx];
       node != 0;
       node = node->next){
    if (node->key == key){
      result = &node->data;
      break;
    }
  }
  
  return(result);
}

// scope stack
static void
pdbconv_symbol_push_scope(PDBCONV_Ctx *ctx, RADDBGIC_Scope *scope, RADDBGIC_Symbol *symbol){
  PDBCONV_ScopeNode *node = ctx->scope_node_free;
  if (node == 0){
    node = push_array(ctx->arena, PDBCONV_ScopeNode, 1);
  }
  else{
    SLLStackPop(ctx->scope_node_free);
  }
  SLLStackPush(ctx->scope_stack, node);
  node->scope = scope;
  node->symbol = symbol;
}

static void
pdbconv_symbol_pop_scope(PDBCONV_Ctx *ctx){
  PDBCONV_ScopeNode *node = ctx->scope_stack;
  if (node != 0){
    SLLStackPop(ctx->scope_stack);
    SLLStackPush(ctx->scope_node_free, node);
  }
}

static void
pdbconv_symbol_clear_scope_stack(PDBCONV_Ctx *ctx){
  for (;;){
    PDBCONV_ScopeNode *node = ctx->scope_stack;
    if (node == 0){
      break;
    }
    SLLStackPop(ctx->scope_stack);
    SLLStackPush(ctx->scope_node_free, node);
  }
}

// PDB/C++ name parsing helper

static U64
pdbconv_end_of_cplusplus_container_name(String8 str){
  // NOTE: This finds the index one past the last "::" contained in str.
  //       if no "::" is contained in str, then the returned index is 0.
  //       The intent is that [0,clamp_bot(0,result - 2)) gives the
  //       "container name" and [result,str.size) gives the leaf name.
  U64 result = 0;
  if (str.size >= 2){
    for (U64 i = str.size; i >= 2; i -= 1){
      if (str.str[i - 2] == ':' && str.str[i - 1] == ':'){
        result = i;
        break;
      }
    }
  }
  return(result);
}

// known global set

static U64
pdbconv_known_global_hash(String8 name, U64 voff){
  U64 result = 5381 ^ voff;
  U8 *ptr = name.str;
  U8 *opl = ptr + name.size;
  for (; ptr < opl; ptr += 1){
    result = ((result << 5) + result) + *ptr;
  }
  return(result);
}

static B32
pdbconv_known_global_lookup(PDBCONV_KnownGlobalSet *set, String8 name, U64 voff){
  U64 hash = pdbconv_known_global_hash(name, voff);
  U64 bucket_idx = hash%set->buckets_count;
  
  PDBCONV_KnownGlobalNode *match = 0;
  for (PDBCONV_KnownGlobalNode *node = set->buckets[bucket_idx];
       node != 0;
       node = node->next){
    if (node->hash == hash &&
        node->key_voff == voff &&
        str8_match(node->key_name, name, 0)){
      match = node;
      break;
    }
  }
  
  B32 result = (match != 0);
  return(result);
}

static void
pdbconv_known_global_insert(Arena *arena, PDBCONV_KnownGlobalSet *set, String8 name, U64 voff){
  U64 hash = pdbconv_known_global_hash(name, voff);
  U64 bucket_idx = hash%set->buckets_count;
  
  PDBCONV_KnownGlobalNode *match = 0;
  for (PDBCONV_KnownGlobalNode *node = set->buckets[bucket_idx];
       node != 0;
       node = node->next){
    if (node->hash == hash &&
        node->key_voff == voff &&
        str8_match(node->key_name, name, 0)){
      match = node;
      break;
    }
  }
  
  if (match == 0){
    PDBCONV_KnownGlobalNode *node = push_array(arena, PDBCONV_KnownGlobalNode, 1);
    SLLStackPush(set->buckets[bucket_idx], node);
    node->key_name = push_str8_copy(arena, name);
    node->key_voff = voff;
    node->hash = hash;
    set->global_count += 1;
    set->bucket_collision_count += (node->next != 0);
  }
}

// location info helpers

static RADDBGIC_Location*
pdbconv_location_from_addr_reg_off(PDBCONV_Ctx *ctx,
                                   RADDBGI_RegisterCode reg_code,
                                   U32 reg_byte_size,
                                   U32 reg_byte_pos,
                                   S64 offset,
                                   B32 extra_indirection){
  RADDBGIC_Location *result = 0;
  if (0 <= offset && offset <= (S64)max_U16){
    if (extra_indirection){
      result = raddbgic_location_addr_addr_reg_plus_u16(ctx->root, reg_code, (U16)offset);
    }
    else{
      result = raddbgic_location_addr_reg_plus_u16(ctx->root, reg_code, (U16)offset);
    }
  }
  else{
    Arena *arena = ctx->arena;
    
    RADDBGIC_EvalBytecode bytecode = {0};
    U32 regread_param = RADDBGI_EncodeRegReadParam(reg_code, reg_byte_size, reg_byte_pos);
    raddbgic_bytecode_push_op(arena, &bytecode, RADDBGI_EvalOp_RegRead, regread_param);
    raddbgic_bytecode_push_sconst(arena, &bytecode, offset);
    raddbgic_bytecode_push_op(arena, &bytecode, RADDBGI_EvalOp_Add, 0);
    if (extra_indirection){
      raddbgic_bytecode_push_op(arena, &bytecode, RADDBGI_EvalOp_MemRead, ctx->addr_size);
    }
    
    result = raddbgic_location_addr_bytecode_stream(ctx->root, &bytecode);
  }
  
  return(result);
}

static CV_EncodedFramePtrReg
pdbconv_cv_encoded_fp_reg_from_proc(PDBCONV_Ctx *ctx, RADDBGIC_Symbol *proc, B32 param_base){
  CV_EncodedFramePtrReg result = 0;
  if (proc != 0){
    PDBCONV_FrameProcData *frame_proc = pdbconv_symbol_frame_proc_read(ctx, proc);
    CV_FrameprocFlags flags = frame_proc->flags;
    if (param_base){
      result = CV_FrameprocFlags_ExtractParamBasePointer(flags);
    }
    else{
      result = CV_FrameprocFlags_ExtractLocalBasePointer(flags);
    }
  }
  return(result);
}

static RADDBGI_RegisterCode
pdbconv_reg_code_from_arch_encoded_fp_reg(RADDBGI_Arch arch, CV_EncodedFramePtrReg encoded_reg){
  RADDBGI_RegisterCode result = 0;
  
  switch (arch){
    case RADDBGI_Arch_X86:
    {
      switch (encoded_reg){
        case CV_EncodedFramePtrReg_StackPtr:
        {
          // TODO(allen): support CV_AllReg_VFRAME
          // TODO(allen): error
        }break;
        case CV_EncodedFramePtrReg_FramePtr:
        {
          result = RADDBGI_RegisterCode_X86_ebp;
        }break;
        case CV_EncodedFramePtrReg_BasePtr:
        {
          result = RADDBGI_RegisterCode_X86_ebx;
        }break;
      }
    }break;
    
    case RADDBGI_Arch_X64:
    {
      switch (encoded_reg){
        case CV_EncodedFramePtrReg_StackPtr:
        {
          result = RADDBGI_RegisterCode_X64_rsp;
        }break;
        case CV_EncodedFramePtrReg_FramePtr:
        {
          result = RADDBGI_RegisterCode_X64_rbp;
        }break;
        case CV_EncodedFramePtrReg_BasePtr:
        {
          result = RADDBGI_RegisterCode_X64_r13;
        }break;
      }
    }break;
  }
  
  return(result);
}

static void
pdbconv_location_over_lvar_addr_range(PDBCONV_Ctx *ctx,
                                      RADDBGIC_LocationSet *locset,
                                      RADDBGIC_Location *location,
                                      CV_LvarAddrRange *range,
                                      CV_LvarAddrGap *gaps, U64 gap_count){
  // extract range info
  U64 voff_first = 0;
  U64 voff_opl = 0;
  {
    COFF_SectionHeader *section = pdbconv_sec_header_from_sec_num(ctx, range->sec);
    if (section != 0){
      voff_first = section->voff + range->off;
      voff_opl = voff_first + range->len;
    }
  }
  
  // emit ranges
  CV_LvarAddrGap *gap_ptr = gaps;
  U64 voff_cursor = voff_first;
  for (U64 i = 0; i < gap_count; i += 1, gap_ptr += 1){
    U64 voff_gap_first = voff_first + gap_ptr->off;
    U64 voff_gap_opl   = voff_gap_first + gap_ptr->len;
    if (voff_cursor < voff_gap_first){
      raddbgic_location_set_add_case(ctx->root, locset, voff_cursor, voff_gap_first, location);
    }
    voff_cursor = voff_gap_opl;
  }
  
  if (voff_cursor < voff_opl){
    raddbgic_location_set_add_case(ctx->root, locset, voff_cursor, voff_opl, location);
  }
}

// link names

static void
pdbconv_link_name_save(Arena *arena, PDBCONV_LinkNameMap *map, U64 voff, String8 name){
  U64 hash = (voff >> 3) ^ ((7 & voff) << 6);
  U64 bucket_idx = hash%map->buckets_count;
  
  PDBCONV_LinkNameNode *node = push_array(arena, PDBCONV_LinkNameNode, 1);
  SLLStackPush(map->buckets[bucket_idx], node);
  node->voff = voff;
  node->name = push_str8_copy(arena, name);
  map->link_name_count += 1;
  map->bucket_collision_count += (node->next != 0);
}

static String8
pdbconv_link_name_find(PDBCONV_LinkNameMap *map, U64 voff){
  U64 hash = (voff >> 3) ^ ((7 & voff) << 6);
  U64 bucket_idx = hash%map->buckets_count;
  
  String8 result = {0};
  for (PDBCONV_LinkNameNode *node = map->buckets[bucket_idx];
       node != 0;
       node = node->next){
    if (node->voff == voff){
      result = node->name;
      break;
    }
  }
  
  return(result);
}

////////////////////////////////
//~ Conversion Path

static PDBCONV_Out *
pdbconv_convert(Arena *arena, PDBCONV_Params *params)
{
  PDBCONV_Out *out = push_array(arena, PDBCONV_Out, 1);
  out->good_parse = 1;
  
  // will we try to parse an input file?
  B32 try_parse_input = (params->errors.node_count == 0);
  
#define PARSE_CHECK_ERROR(p,fmt,...) do{ if ((p) == 0){\
out->good_parse = 0;\
str8_list_pushf(arena, &out->errors, fmt, __VA_ARGS__);\
} }while(0)
  
  // parse msf file
  MSF_Parsed *msf = 0;
  if (try_parse_input) ProfScope("parse msf"){
    msf = msf_parsed_from_data(arena, params->input_pdb_data);
    PARSE_CHECK_ERROR(msf, "MSF");
  }
  
  // parse pdb info
  PDB_NamedStreamTable *named_streams = 0;
  COFF_Guid auth_guid = {0};
  if (msf != 0) ProfScope("parse pdb info"){
    Temp scratch = scratch_begin(&arena, 1);
    
    String8 info_data = msf_data_from_stream(msf, PDB_FixedStream_PdbInfo);
    PDB_Info *info = pdb_info_from_data(scratch.arena, info_data);
    named_streams = pdb_named_stream_table_from_info(arena, info);
    MemoryCopyStruct(&auth_guid, &info->auth_guid);
    
    scratch_end(scratch);
    
    PARSE_CHECK_ERROR(named_streams, "named streams from pdb info");
  }
  
  // parse strtbl
  PDB_Strtbl *strtbl = 0;
  if (named_streams != 0) ProfScope("parse strtbl"){
    MSF_StreamNumber strtbl_sn = named_streams->sn[PDB_NamedStream_STRTABLE];
    String8 strtbl_data = msf_data_from_stream(msf, strtbl_sn);
    strtbl = pdb_strtbl_from_data(arena, strtbl_data);
    
    PARSE_CHECK_ERROR(strtbl, "string table");
  }
  
  // parse dbi
  PDB_DbiParsed *dbi = 0;
  if (msf != 0) ProfScope("parse dbi"){
    String8 dbi_data = msf_data_from_stream(msf, PDB_FixedStream_Dbi);
    dbi = pdb_dbi_from_data(arena, dbi_data);
    
    PARSE_CHECK_ERROR(dbi, "DBI");
  }
  
  // parse tpi
  PDB_TpiParsed *tpi = 0;
  if (msf != 0) ProfScope("parse tpi"){
    String8 tpi_data = msf_data_from_stream(msf, PDB_FixedStream_Tpi);
    tpi = pdb_tpi_from_data(arena, tpi_data);
    
    PARSE_CHECK_ERROR(tpi, "TPI");
  }
  
  // parse ipi
  PDB_TpiParsed *ipi = 0;
  if (msf != 0) ProfScope("parse ipi"){
    String8 ipi_data = msf_data_from_stream(msf, PDB_FixedStream_Ipi);
    ipi = pdb_tpi_from_data(arena, ipi_data);
    
    PARSE_CHECK_ERROR(ipi, "IPI");
  }
  
  // parse coff sections
  PDB_CoffSectionArray *coff_sections = 0;
  U64 coff_section_count = 0;
  if (dbi != 0) ProfScope("parse coff sections"){
    MSF_StreamNumber section_stream = dbi->dbg_streams[PDB_DbiStream_SECTION_HEADER];
    String8 section_data = msf_data_from_stream(msf, section_stream);
    coff_sections = pdb_coff_section_array_from_data(arena, section_data);
    coff_section_count = coff_sections->count;
    
    PARSE_CHECK_ERROR(coff_sections, "coff sections");
  }
  
  // parse gsi
  PDB_GsiParsed *gsi = 0;
  if (dbi != 0) ProfScope("parse gsi"){
    String8 gsi_data = msf_data_from_stream(msf, dbi->gsi_sn);
    gsi = pdb_gsi_from_data(arena, gsi_data);
    
    PARSE_CHECK_ERROR(gsi, "GSI");
  }
  
  // parse psi
  PDB_GsiParsed *psi_gsi_part = 0;
  if (dbi != 0) ProfScope("parse psi"){
    String8 psi_data = msf_data_from_stream(msf, dbi->psi_sn);
    String8 psi_data_gsi_part = str8_range(psi_data.str + sizeof(PDB_PsiHeader),
                                           psi_data.str + psi_data.size);
    psi_gsi_part = pdb_gsi_from_data(arena, psi_data_gsi_part);
    
    PARSE_CHECK_ERROR(psi_gsi_part, "PSI");
  }
  
  // parse tpi hash
  PDB_TpiHashParsed *tpi_hash = 0;
  if (tpi != 0) ProfScope("parse tpi hash"){
    String8 hash_data = msf_data_from_stream(msf, tpi->hash_sn);
    String8 aux_data = msf_data_from_stream(msf, tpi->hash_sn_aux);
    tpi_hash = pdb_tpi_hash_from_data(arena, strtbl, tpi, hash_data, aux_data);
    
    PARSE_CHECK_ERROR(tpi_hash, "TPI hash table");
  }
  
  // parse tpi leaves
  CV_LeafParsed *tpi_leaf = 0;
  if (tpi != 0) ProfScope("parse tpi leaves"){
    String8 leaf_data = pdb_leaf_data_from_tpi(tpi);
    tpi_leaf = cv_leaf_from_data(arena, leaf_data, tpi->itype_first);
    
    PARSE_CHECK_ERROR(tpi_hash, "TPI leaf data");
  }
  
  // parse ipi hash
  PDB_TpiHashParsed *ipi_hash = 0;
  if (ipi != 0) ProfScope("parse ipi hash"){
    String8 hash_data = msf_data_from_stream(msf, ipi->hash_sn);
    String8 aux_data = msf_data_from_stream(msf, ipi->hash_sn_aux);
    ipi_hash = pdb_tpi_hash_from_data(arena, strtbl, ipi, hash_data, aux_data);
    
    PARSE_CHECK_ERROR(tpi_hash, "IPI hash table");
  }
  
  // parse ipi leaves
  CV_LeafParsed *ipi_leaf = 0;
  if (ipi != 0) ProfScope("parse ipi leaves"){
    String8 leaf_data = pdb_leaf_data_from_tpi(ipi);
    ipi_leaf = cv_leaf_from_data(arena, leaf_data, ipi->itype_first);
    
    PARSE_CHECK_ERROR(tpi_hash, "IPI leaf data");
  }
  
  // parse sym
  CV_SymParsed *sym = 0;
  if (dbi != 0) ProfScope("parse sym"){
    String8 sym_data = msf_data_from_stream(msf, dbi->sym_sn);
    sym = cv_sym_from_data(arena, sym_data, 4);
    
    PARSE_CHECK_ERROR(tpi_hash, "public SYM data");
  }
  
  // parse compilation units
  PDB_CompUnitArray *comp_units = 0;
  U64 comp_unit_count = 0;
  if (dbi != 0) ProfScope("parse compilation units"){
    String8 mod_info_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_ModuleInfo);
    comp_units = pdb_comp_unit_array_from_data(arena, mod_info_data);
    comp_unit_count = comp_units->count;
    
    PARSE_CHECK_ERROR(comp_units, "module info");
  }
  
  // parse dbi's section contributions
  PDB_CompUnitContributionArray *comp_unit_contributions = 0;
  U64 comp_unit_contribution_count = 0;
  if (dbi != 0 && coff_sections != 0) ProfScope("parse dbi section contributions"){
    String8 section_contribution_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_SecCon);
    comp_unit_contributions =
      pdb_comp_unit_contribution_array_from_data(arena, section_contribution_data, coff_sections);
    comp_unit_contribution_count = comp_unit_contributions->count;
    
    PARSE_CHECK_ERROR(comp_unit_contributions, "module contributions");
  }
  
  // parse syms for each compilation unit
  CV_SymParsed **sym_for_unit = push_array(arena, CV_SymParsed*, comp_unit_count);
  if (comp_units != 0) ProfScope("parse symbols"){
    PDB_CompUnit **unit_ptr = comp_units->units;
    for (U64 i = 0; i < comp_unit_count; i += 1, unit_ptr += 1){
      CV_SymParsed *unit_sym = 0;
      {
        String8 sym_data = pdb_data_from_unit_range(msf, *unit_ptr, PDB_DbiCompUnitRange_Symbols);
        unit_sym = cv_sym_from_data(arena, sym_data, 4);
      }
      PARSE_CHECK_ERROR(unit_sym, "module (i=%llu) SYM data", i);
      
      sym_for_unit[i] = unit_sym;
    }
  }
  
  // parse c13 for each compilation unit
  CV_C13Parsed **c13_for_unit = push_array(arena, CV_C13Parsed*, comp_unit_count);
  if (comp_units != 0) ProfScope("parse c13s"){
    PDB_CompUnit **unit_ptr = comp_units->units;
    for (U64 i = 0; i < comp_unit_count; i += 1, unit_ptr += 1){
      CV_C13Parsed *unit_c13 = 0;
      {
        String8 c13_data = pdb_data_from_unit_range(msf, *unit_ptr, PDB_DbiCompUnitRange_C13);
        unit_c13 = cv_c13_from_data(arena, c13_data, strtbl, coff_sections);
      }
      PARSE_CHECK_ERROR(unit_c13, "module (i=%llu) C13 line info", i);
      
      c13_for_unit[i] = unit_c13;
    }
  }
  
  // parsing error
  if (try_parse_input && !out->good_parse &&
      !params->hide_errors.parsing){
    str8_list_pushf(arena, &out->errors, "error(parsing): '%S' as a PDB\n", params->input_pdb_name);
  }
  
  // exe hash
  U64 exe_hash = 0;
  if (out->good_parse && params->input_exe_data.size > 0) ProfScope("hash exe"){
    exe_hash = raddbgi_hash(params->input_exe_data.str, params->input_exe_data.size);
  }
  
  // output generation
  PDBCONV_Ctx *pdbconv_ctx = 0;
  if (params->output_name.size > 0){
    
    // determine arch
    RADDBGI_Arch architecture = RADDBGI_Arch_NULL;
    // TODO(rjf): in some cases, the first compilation unit has a zero
    // architecture, as it's sometimes used as a "nil" unit. this causes bugs
    // in later stages of conversion - particularly, this was detected via
    // busted location info. so i've converted this to a scan-until-we-find-an-
    // architecture. however, this is still fundamentally insufficient, because
    // Nick has informed me that x86 units can be linked with x64 units,
    // meaning the appropriate architecture at any point in time is not a top-
    // level concept, and is rather dependent on to which compilation unit
    // particular symbols belong. so in the future, to support that (odd) case,
    // we'll need to not only have this be a top-level "contextual" piece of
    // info, but to use the appropriate compilation unit's architecture when
    // possible.
    //
    for(U64 comp_unit_idx = 0; comp_unit_idx < comp_unit_count; comp_unit_idx += 1)
    {
      if(sym_for_unit[comp_unit_idx] != 0)
      {
        architecture = raddbgi_arch_from_cv_arch(sym_for_unit[comp_unit_idx]->info.arch);
        if(architecture != 0)
        {
          break;
        }
      }
    }
    U64 addr_size = raddbgi_addr_size_from_arch(architecture);
    
    
    // predict symbol counts
    U64 symbol_count_prediction = 0;
    {
      U64 rec_range_count = 0;
      if (sym != 0){
        rec_range_count += sym->sym_ranges.count;
      }
      for (U64 i = 0; i < comp_unit_count; i += 1){
        CV_SymParsed *unit_sym = sym_for_unit[i];
        rec_range_count += unit_sym->sym_ranges.count;
      }
      symbol_count_prediction = rec_range_count/8;
      if (symbol_count_prediction < 128){
        symbol_count_prediction = 128;
      }
    }
    
    
    // setup root
    RADDBGIC_RootParams root_params = {0};
    root_params.addr_size = addr_size;
    
    root_params.bucket_count_units = comp_unit_count;
    root_params.bucket_count_symbols = symbol_count_prediction;
    root_params.bucket_count_scopes = symbol_count_prediction;
    root_params.bucket_count_locals = symbol_count_prediction*2;
    root_params.bucket_count_types = tpi->itype_opl;
    root_params.bucket_count_type_constructs = tpi->itype_opl;
    
    RADDBGIC_Root *root = raddbgic_root_alloc(&root_params);
    out->root = root;
    
    // top level info
    {
      // calculate voff max
      U64 voff_max = 0;
      {        
        COFF_SectionHeader *coff_sec_ptr = coff_sections->sections;
        COFF_SectionHeader *coff_ptr_opl = coff_sec_ptr + coff_section_count;
        for (;coff_sec_ptr < coff_ptr_opl; coff_sec_ptr += 1){
          U64 sec_voff_max = coff_sec_ptr->voff + coff_sec_ptr->vsize;
          voff_max = Max(voff_max, sec_voff_max);
        }
      }
      
      // set top level info
      RADDBGIC_TopLevelInfo tli = {0};
      tli.architecture = architecture;
      tli.exe_name = params->input_exe_name;
      tli.exe_hash = exe_hash;
      tli.voff_max = voff_max;
      
      raddbgic_set_top_level_info(root, &tli);
    }
    
    
    // setup binary sections
    {
      COFF_SectionHeader *coff_ptr = coff_sections->sections;
      COFF_SectionHeader *coff_opl = coff_ptr + coff_section_count;
      for (;coff_ptr < coff_opl; coff_ptr += 1){
        char *name_first = (char*)coff_ptr->name;
        char *name_opl   = name_first + sizeof(coff_ptr->name);
        String8 name = str8_cstring_capped(name_first, name_opl);
        RADDBGI_BinarySectionFlags flags =
          raddbgi_binary_section_flags_from_coff_section_flags(coff_ptr->flags);
        raddbgic_add_binary_section(root, name, flags,
                                    coff_ptr->voff, coff_ptr->voff + coff_ptr->vsize,
                                    coff_ptr->foff, coff_ptr->foff + coff_ptr->fsize);
      }
    }
    
    
    // setup compilation units
    {
      PDB_CompUnit **units = comp_units->units;
      for (U64 i = 0; i < comp_unit_count; i += 1){
        PDB_CompUnit *unit = units[i];
        CV_SymParsed *unit_sym = sym_for_unit[i];
        CV_C13Parsed *unit_c13 = c13_for_unit[i];
        
        // resolve names
        String8 raw_name = unit->obj_name;
        
        String8 unit_name = raw_name;
        {
          U64 first_after_slashes = 0;
          for (S64 i = unit_name.size - 1; i >= 0; i -= 1){
            if (unit_name.str[i] == '/' || unit_name.str[i] == '\\'){
              first_after_slashes = i + 1;
              break;
            }
          }
          unit_name = str8_range(raw_name.str + first_after_slashes,
                                 raw_name.str + raw_name.size);
        }
        
        String8 obj_name = raw_name;
        if (str8_match(obj_name, str8_lit("* Linker *"), 0) ||
            str8_match(obj_name, str8_lit("Import:"),
                       StringMatchFlag_RightSideSloppy)){
          MemoryZeroStruct(&obj_name);
        }
        
        String8 compiler_name = unit_sym->info.compiler_name;
        String8 archive_file = unit->group_name;
        
        // extract langauge
        RADDBGI_Language lang = raddbgi_language_from_cv_language(sym->info.language);
        
        // basic per unit info
        RADDBGIC_Unit *unit_handle = raddbgic_unit_handle_from_user_id(root, i, i);
        
        RADDBGIC_UnitInfo info = {0};
        info.unit_name = unit_name;
        info.compiler_name = compiler_name;
        info.object_file = obj_name;
        info.archive_file = archive_file;
        info.language = lang;
        
        raddbgic_unit_set_info(root, unit_handle, &info);
        
        // unit's line info
        for (CV_C13SubSectionNode *node = unit_c13->first_sub_section;
             node != 0;
             node = node->next)
        {
          if(node->kind == CV_C13_SubSectionKind_Lines)
          {
            for(CV_C13LinesParsedNode *lines_n = node->lines_first;
                lines_n != 0;
                lines_n = lines_n->next)
            {
              CV_C13LinesParsed *lines = &lines_n->v;
              RADDBGIC_LineSequence seq = {0};
              seq.file_name  = lines->file_name;
              seq.voffs      = lines->voffs;
              seq.line_nums  = lines->line_nums;
              seq.col_nums   = lines->col_nums;
              seq.line_count = lines->line_count;
              raddbgic_unit_add_line_sequence(root, unit_handle, &seq);
            }
          }
        }
      }
    }
    
    
    // unit vmap ranges
    {
      PDB_CompUnitContribution *contrib_ptr = comp_unit_contributions->contributions;
      PDB_CompUnitContribution *contrib_opl = contrib_ptr + comp_unit_contribution_count;
      for (;contrib_ptr < contrib_opl; contrib_ptr += 1){
        if (contrib_ptr->mod < root->unit_count){
          RADDBGIC_Unit *unit_handle = raddbgic_unit_handle_from_user_id(root, contrib_ptr->mod, contrib_ptr->mod);
          raddbgic_unit_vmap_add_range(root, unit_handle,
                                       contrib_ptr->voff_first,
                                       contrib_ptr->voff_opl);
        }
      }
    }
    
    // rjf: produce pdb conversion context
    {
      PDBCONV_CtxParams p = {0};
      {
        p.arch = architecture;
        p.tpi_hash = tpi_hash;
        p.tpi_leaf = tpi_leaf;
        p.sections = coff_sections;
        p.fwd_map_bucket_count          = tpi->itype_opl/10;
        p.frame_proc_map_bucket_count   = symbol_count_prediction;
        p.known_global_map_bucket_count = symbol_count_prediction;
        p.link_name_map_bucket_count    = symbol_count_prediction;
      }
      pdbconv_ctx = pdbconv_ctx_alloc(&p, root);
    }
    
    // types & symbols
    {
      PDBCONV_TypesSymbolsParams p = {0};
      p.sym = sym;
      p.sym_for_unit = sym_for_unit;
      p.unit_count = comp_unit_count;
      pdbconv_types_and_symbols(pdbconv_ctx, &p);
    }
    
    // conversion errors
    if (!params->hide_errors.converting){
      for (RADDBGIC_Error *error = raddbgic_first_error_from_root(root);
           error != 0;
           error = error->next){
        str8_list_push(arena, &out->errors, error->msg);
      }
    }
  }
  
  // dump
  if (params->dump) ProfScope("dump"){
    String8List dump = {0};
    
    // EXE
    if (out->good_parse){
      str8_list_push(arena, &dump,
                     str8_lit("################################"
                              "################################\n"
                              "EXE INFO:\n"));
      {
        str8_list_pushf(arena, &dump, "HASH: %016llX\n", exe_hash);
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    // MSF
    if (params->dump_msf){
      if (msf != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "MSF:\n"));
        
        str8_list_pushf(arena, &dump, " block_size=%llu\n", msf->block_size);
        str8_list_pushf(arena, &dump, " block_count=%llu\n", msf->block_count);
        str8_list_pushf(arena, &dump, " stream_count=%llu\n", msf->stream_count);
        
        String8 *stream_ptr = msf->streams;
        U64 stream_count = msf->stream_count;
        for (U64 i = 0; i < stream_count; i += 1, stream_ptr += 1){
          str8_list_pushf(arena, &dump, "  stream[%u].size=%llu\n",
                          i, stream_ptr->size);
        }
        
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
    }
    
    // DBI
    if (params->dump_sym){
      if (sym != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "DBI SYM:\n"));
        cv_stringize_sym_parsed(arena, &dump, sym);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
    }
    
    // TPI
    if (params->dump_tpi_hash){
      if (tpi_hash != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "TPI HASH:\n"));
        pdb_stringize_tpi_hash(arena, &dump, tpi_hash);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      
      if (ipi_hash != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "IPI HASH:\n"));
        pdb_stringize_tpi_hash(arena, &dump, ipi_hash);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
    }
    
    // LEAF
    if (params->dump_leaf){
      if (tpi_leaf != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "TPI LEAF:\n"));
        cv_stringize_leaf_parsed(arena, &dump, tpi_leaf);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
      
      if (ipi_leaf != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "IPI LEAF:\n"));
        cv_stringize_leaf_parsed(arena, &dump, ipi_leaf);
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
    }
    
    // BINARY SECTIONS
    if (params->dump_coff_sections){
      if (coff_sections != 0){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "COFF SECTIONS:\n"));
        COFF_SectionHeader *section_ptr = coff_sections->sections;
        for (U64 i = 0; i < coff_section_count; i += 1, section_ptr += 1){
          // TODO(allen): probably should pull this out into a separate stringize path
          // for the coff section type
          char *first = (char*)section_ptr->name;
          char *opl   = first + sizeof(section_ptr->name);
          String8 name = str8_cstring_capped(first, opl);
          str8_list_pushf(arena, &dump, " %.*s:\n", str8_varg(name));
          str8_list_pushf(arena, &dump, "  vsize=%u\n", section_ptr->vsize);
          str8_list_pushf(arena, &dump, "  voff =0x%x\n", section_ptr->voff);
          str8_list_pushf(arena, &dump, "  fsize=%u\n", section_ptr->fsize);
          str8_list_pushf(arena, &dump, "  foff =0x%x\n", section_ptr->foff);
          str8_list_pushf(arena, &dump, "  relocs_foff=0x%x\n", section_ptr->relocs_foff);
          str8_list_pushf(arena, &dump, "  lines_foff =0x%x\n", section_ptr->lines_foff);
          str8_list_pushf(arena, &dump, "  reloc_count=%u\n", section_ptr->reloc_count);
          str8_list_pushf(arena, &dump, "  line_count =%u\n", section_ptr->line_count);
          // TODO(allen): better flags
          str8_list_pushf(arena, &dump, "  flags=%x\n", section_ptr->flags);
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
      }
    }
    
    // UNITS
    if (comp_units != 0){
      B32 dump_sym = params->dump_sym;
      B32 dump_c13 = params->dump_c13;
      
      B32 dump_units = (dump_sym || dump_c13);
      
      if (dump_units){
        PDB_CompUnit **unit_ptr = comp_units->units;
        for (U64 i = 0; i < comp_unit_count; i += 1, unit_ptr += 1){
          str8_list_push(arena, &dump,
                         str8_lit("################################"
                                  "################################\n"));
          String8 name = (*unit_ptr)->obj_name;
          String8 group_name = (*unit_ptr)->group_name;
          str8_list_pushf(arena, &dump, "[%llu] %.*s\n(%.*s):\n",
                          i, str8_varg(name), str8_varg(group_name));
          if (dump_sym){
            cv_stringize_sym_parsed(arena, &dump, sym_for_unit[i]);
          }
          if (dump_c13){
            cv_stringize_c13_parsed(arena, &dump, c13_for_unit[i]);
          }
          str8_list_push(arena, &dump, str8_lit("\n"));
        }
      }
    }
    
    // UNIT CONTRIBUTIONS
    if (comp_unit_contributions != 0){
      if (params->dump_contributions){
        str8_list_push(arena, &dump,
                       str8_lit("################################"
                                "################################\n"
                                "UNIT CONTRIBUTIONS:\n"));
        PDB_CompUnitContribution *contrib_ptr = comp_unit_contributions->contributions;
        for (U64 i = 0; i < comp_unit_contribution_count; i += 1, contrib_ptr += 1){
          str8_list_pushf(arena, &dump,
                          " { mod = %5u; voff_first = %08llx; voff_opl = %08llx; }\n",
                          contrib_ptr->mod, contrib_ptr->voff_first, contrib_ptr->voff_opl);
        }
        str8_list_push(arena, &dump, str8_lit("\n"));
      }
    }
    
    // rjf: dump table diagnostics
    if(params->dump_table_diagnostics)
    {
      str8_list_push(arena, &dump,
                     str8_lit("################################"
                              "################################\n"
                              "TABLE DIAGNOSTICS:\n"));
      struct
      {
        String8 name;
        U64 bucket_count;
        U64 value_count;
        U64 collision_count;
      }
      table_info[] =
      {
        {str8_lit("pdbconv_ctx fwd_map"),        pdbconv_ctx?pdbconv_ctx->fwd_map.buckets_count:0,         pdbconv_ctx?pdbconv_ctx->fwd_map.pair_count:0,         pdbconv_ctx?pdbconv_ctx->fwd_map.bucket_collision_count:0},
        {str8_lit("pdbconv_ctx frame_proc_map"), pdbconv_ctx?pdbconv_ctx->frame_proc_map.buckets_count:0,  pdbconv_ctx?pdbconv_ctx->frame_proc_map.pair_count:0,  pdbconv_ctx?pdbconv_ctx->frame_proc_map.bucket_collision_count:0},
        {str8_lit("pdbconv_ctx known_globals"),  pdbconv_ctx?pdbconv_ctx->known_globals.buckets_count:0,   pdbconv_ctx?pdbconv_ctx->known_globals.global_count:0, pdbconv_ctx?pdbconv_ctx->known_globals.bucket_collision_count:0},
        {str8_lit("pdbconv_ctx link_names"),     pdbconv_ctx?pdbconv_ctx->link_names.buckets_count:0,      pdbconv_ctx?pdbconv_ctx->link_names.link_name_count:0, pdbconv_ctx?pdbconv_ctx->link_names.bucket_collision_count:0},
        {str8_lit("raddbgic_root unit_map"),         out->root->unit_map.buckets_count,          out->root->unit_map.pair_count,          out->root->unit_map.bucket_collision_count},
        {str8_lit("raddbgic_root symbol_map"),       out->root->symbol_map.buckets_count,        out->root->symbol_map.pair_count,        out->root->symbol_map.bucket_collision_count},
        {str8_lit("raddbgic_root scope_map"),        out->root->scope_map.buckets_count,         out->root->scope_map.pair_count,         out->root->scope_map.bucket_collision_count},
        {str8_lit("raddbgic_root local_map"),        out->root->local_map.buckets_count,         out->root->local_map.pair_count,         out->root->local_map.bucket_collision_count},
        {str8_lit("raddbgic_root type_from_id_map"), out->root->type_from_id_map.buckets_count,  out->root->type_from_id_map.pair_count,  out->root->type_from_id_map.bucket_collision_count},
        {str8_lit("raddbgic_root construct_map"),    out->root->construct_map.buckets_count,     out->root->construct_map.pair_count,     out->root->construct_map.bucket_collision_count},
      };
      for(U64 idx = 0; idx < ArrayCount(table_info); idx += 1)
      {
        str8_list_pushf(arena, &dump, "%S: %I64u values in %I64u buckets, with %I64u collisions (%f fill)\n",
                        table_info[idx].name,
                        table_info[idx].value_count,
                        table_info[idx].bucket_count,
                        table_info[idx].collision_count,
                        (F64)table_info[idx].value_count / (F64)table_info[idx].bucket_count);
      }
      str8_list_push(arena, &dump, str8_lit("\n"));
    }
    
    out->dump = dump;
  }
  
  return out;
}
