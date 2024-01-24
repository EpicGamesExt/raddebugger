// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBG_FROM_PDB_H
#define RADDBG_FROM_PDB_H

////////////////////////////////
//~ Program Parameters Type

typedef struct PDBCONV_Params{
  String8 input_pdb_name;
  String8 input_pdb_data;
  
  String8 input_exe_name;
  String8 input_exe_data;
  
  String8 output_name;
  
  struct{
    B8 input;
    B8 output;
    B8 parsing;
    B8 converting;
  } hide_errors;
  
  B8 dump;
  B8 dump__first;
  B8 dump_coff_sections;
  B8 dump_msf;
  B8 dump_sym;
  B8 dump_tpi_hash;
  B8 dump_leaf;
  B8 dump_c13;
  B8 dump_contributions;
  B8 dump__last;
  
  String8List errors;
} PDBCONV_Params;

////////////////////////////////
//~ Program Parameters Parser

static PDBCONV_Params *pdb_convert_params_from_cmd_line(Arena *arena, CmdLine *cmdline);

////////////////////////////////
//~ PDB Type & Symbol Info Translation Helpers

//- translation helper types
typedef struct PDBCONV_FwdNode{
  struct PDBCONV_FwdNode *next;
  CV_TypeId key;
  CV_TypeId val;
} PDBCONV_FwdNode;

typedef struct PDBCONV_FwdMap{
  PDBCONV_FwdNode *buckets[1<<24];
} PDBCONV_FwdMap;

typedef struct PDBCONV_TypeRev{
  struct PDBCONV_TypeRev *next;
  CONS_Type *owner_type;
  CV_TypeId field_itype;
} PDBCONV_TypeRev;

typedef struct PDBCONV_FrameProcData{
  U32 frame_size;
  CV_FrameprocFlags flags;
} PDBCONV_FrameProcData;

typedef struct PDBCONV_FrameProcNode{
  struct PDBCONV_FrameProcNode *next;
  CONS_Symbol *key;
  PDBCONV_FrameProcData data;
} PDBCONV_FrameProcNode;

typedef struct PDBCONV_FrameProcMap{
  PDBCONV_FrameProcNode *buckets[1<<24];
} PDBCONV_FrameProcMap;

typedef struct PDBCONV_ScopeNode{
  struct PDBCONV_ScopeNode *next;
  CONS_Scope *scope;
  CONS_Symbol *symbol;
} PDBCONV_ScopeNode;

typedef struct PDBCONV_KnownGlobalNode{
  struct PDBCONV_KnownGlobalNode *next;
  String8 key_name;
  U64     key_voff;
  U64     hash;
} PDBCONV_KnownGlobalNode;

typedef struct PDBCONV_KnownGlobalSet{
  PDBCONV_KnownGlobalNode *buckets[1<<24];
} PDBCONV_KnownGlobalSet;

typedef struct PDBCONV_TypesSymbolsParams{
  RADDBG_Arch architecture;
  CV_SymParsed *sym;
  CV_SymParsed **sym_for_unit;
  U64 unit_count;
  PDB_TpiHashParsed *tpi_hash;
  CV_LeafParsed *tpi_leaf;
  PDB_CoffSectionArray *sections;
} PDBCONV_TypesSymbolsParams;

typedef struct PDBCONV_LinkNameNode{
  struct PDBCONV_LinkNameNode *next;
  U64 voff;
  String8 name;
} PDBCONV_LinkNameNode;

typedef struct PDBCONV_LinkNameMap{
  PDBCONV_LinkNameNode *buckets[1<<24];
} PDBCONV_LinkNameMap;

typedef struct PDBCONV_Ctx{
  // INPUT data
  RADDBG_Arch arch;
  U64 addr_size;
  PDB_TpiHashParsed *hash;
  CV_LeafParsed *leaf;
  COFF_SectionHeader *sections;
  U64 section_count;
  
  // OUTPUT data
  CONS_Root *root;
  
  // TEMPORARY STATE
  Arena *temp_arena;
  PDBCONV_FwdMap fwd_map;
  PDBCONV_TypeRev *member_revisit_first;
  PDBCONV_TypeRev *member_revisit_last;
  PDBCONV_TypeRev *enum_revisit_first;
  PDBCONV_TypeRev *enum_revisit_last;
  PDBCONV_FrameProcMap frame_proc_map;
  PDBCONV_ScopeNode *scope_stack;
  PDBCONV_ScopeNode *scope_node_free;
  PDBCONV_KnownGlobalSet known_globals;
  PDBCONV_LinkNameMap link_names;
} PDBCONV_Ctx;

//- pdb types and symbols
static void pdbconv_types_and_symbols(PDBCONV_TypesSymbolsParams *params, CONS_Root *out_root);

//- decoding helpers
static U32 pdbconv_u32_from_numeric(PDBCONV_Ctx *ctx, CV_NumericParsed *num);
static COFF_SectionHeader* pdbconv_sec_header_from_sec_num(PDBCONV_Ctx *ctx, U32 sec_num);

//- type info

// TODO(allen): explain the overarching pattern of PDB type info translation here
// 1. main passes (out of order necessity) & after
// 2. resolve forward
// 3. cons type info
// 4. "resolve itype"
// 5. equipping members & enumerates
// 6. equipping source coordinates

// type info construction passes
static void       pdbconv_type_cons_main_passes(PDBCONV_Ctx *ctx);

static CV_TypeId  pdbconv_type_resolve_fwd(PDBCONV_Ctx *ctx, CV_TypeId itype);
static CONS_Type* pdbconv_type_resolve_itype(PDBCONV_Ctx *ctx, CV_TypeId itype);
static void       pdbconv_type_equip_members(PDBCONV_Ctx *ctx, CONS_Type *owern_type,
                                             CV_TypeId field_itype);
static void       pdbconv_type_equip_enumerates(PDBCONV_Ctx *ctx, CONS_Type *owner_type,
                                                CV_TypeId field_itype);

// type info construction helpers
static CONS_Type* pdbconv_type_cons_basic(PDBCONV_Ctx *ctx, CV_TypeId itype);
static CONS_Type* pdbconv_type_cons_leaf_record(PDBCONV_Ctx *ctx, CV_TypeId itype);
static CONS_Type* pdbconv_type_resolve_and_check(PDBCONV_Ctx *ctx, CV_TypeId itype);
static void       pdbconv_type_resolve_arglist(Arena *arena, CONS_TypeList *out,
                                               PDBCONV_Ctx *ctx, CV_TypeId arglist_itype);

// type info resolution helpers
static CONS_Type* pdbconv_type_from_name(PDBCONV_Ctx *ctx, String8 name);

// type fwd map
static void      pdbconv_type_fwd_map_set(Arena *arena, PDBCONV_FwdMap *map,
                                          CV_TypeId key, CV_TypeId val);
static CV_TypeId pdbconv_type_fwd_map_get(PDBCONV_FwdMap *map, CV_TypeId key);


//- symbol info

// symbol info construction
static void pdbconv_symbol_cons(PDBCONV_Ctx *ctx, CV_SymParsed *sym, U32 sym_unique_id);
static void pdbconv_gather_link_names(PDBCONV_Ctx *ctx, CV_SymParsed *sym);

// "frameproc" map
static void                   pdbconv_symbol_frame_proc_write(PDBCONV_Ctx *ctx,CONS_Symbol *key,
                                                              PDBCONV_FrameProcData *data);
static PDBCONV_FrameProcData* pdbconv_symbol_frame_proc_read(PDBCONV_Ctx *ctx, CONS_Symbol *key);

// scope stack
static void pdbconv_symbol_push_scope(PDBCONV_Ctx *ctx, CONS_Scope *scope, CONS_Symbol *symbol);
static void pdbconv_symbol_pop_scope(PDBCONV_Ctx *ctx);
static void pdbconv_symbol_clear_scope_stack(PDBCONV_Ctx *ctx);

#define pdbconv_symbol_current_scope(ctx) \
((ctx)->scope_stack == 0)?0:((ctx)->scope_stack->scope)

#define pdbconv_symbol_current_symbol(ctx) \
((ctx)->scope_stack == 0)?0:((ctx)->scope_stack->symbol)

// PDB/C++ name parsing helper
static U64 pdbconv_end_of_cplusplus_container_name(String8 str);

// global deduplication
static U64  pdbconv_known_global_hash(String8 name, U64 voff);

static B32  pdbconv_known_global_lookup(PDBCONV_KnownGlobalSet *set, String8 name, U64 voff);
static void pdbconv_known_global_insert(Arena *arena, PDBCONV_KnownGlobalSet *set,
                                        String8 name, U64 voff);


// location info helpers
static CONS_Location* pdbconv_location_from_addr_reg_off(PDBCONV_Ctx *ctx,
                                                         RADDBG_RegisterCode reg_code,
                                                         U32 reg_byte_size,
                                                         U32 reg_byte_pos,
                                                         S64 offset,
                                                         B32 extra_indirection);

static CV_EncodedFramePtrReg pdbconv_cv_encoded_fp_reg_from_proc(PDBCONV_Ctx *ctx,
                                                                 CONS_Symbol *proc,
                                                                 B32 param_base);

static RADDBG_RegisterCode pdbconv_reg_code_from_arch_encoded_fp_reg(RADDBG_Arch arch,
                                                                     CV_EncodedFramePtrReg encoded_reg);

static void pdbconv_location_over_lvar_addr_range(PDBCONV_Ctx *ctx,
                                                  CONS_LocationSet *locset,
                                                  CONS_Location *location,
                                                  CV_LvarAddrRange *range,
                                                  CV_LvarAddrGap *gaps, U64 gap_count);

// link names
static void    pdbconv_link_name_save(Arena *arena, PDBCONV_LinkNameMap *map,
                                      U64 voff, String8 name);
static String8 pdbconv_link_name_find(PDBCONV_LinkNameMap *map, U64 voff);

////////////////////////////////
//~ Conversion Output Type

typedef struct PDBCONV_Out PDBCONV_Out;
struct PDBCONV_Out
{
  B32 good_parse;
  CONS_Root *root;
  String8List dump;
  String8List errors;
};

////////////////////////////////
//~ Conversion Path

static PDBCONV_Out *pdbconv_convert(Arena *arena, PDBCONV_Params *params);

#endif //RADDBG_FROM_PDB_H
