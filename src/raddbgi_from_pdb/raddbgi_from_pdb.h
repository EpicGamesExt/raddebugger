// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RADDBGI_FROM_PDB_H
#define RADDBGI_FROM_PDB_H

////////////////////////////////
//~ rjf: Conversion Parameters Type

typedef struct P2R_Params P2R_Params;
struct P2R_Params
{
  String8 input_pdb_name;
  String8 input_pdb_data;
  String8 input_exe_name;
  String8 input_exe_data;
  String8 output_name;
  
  struct
  {
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
  B8 dump_table_diagnostics;
  B8 dump__last;
  
  String8List errors;
};

////////////////////////////////
//~ rjf: PDB Type & Symbol Info Translation Helper Types

//- rjf: typeid forward reference map

typedef struct P2R_FwdNode P2R_FwdNode;
struct P2R_FwdNode
{
  P2R_FwdNode *next;
  CV_TypeId key;
  CV_TypeId val;
};

typedef struct P2R_FwdMap P2R_FwdMap;
struct P2R_FwdMap
{
  P2R_FwdNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 pair_count;
};

//- rjf: type revisit lists

typedef struct P2R_TypeRev P2R_TypeRev;
struct P2R_TypeRev
{
  P2R_TypeRev *next;
  RADDBGIC_Type *owner_type;
  CV_TypeId field_itype;
};

//- rjf: frame proc maps

typedef struct P2R_FrameProcData P2R_FrameProcData;
struct P2R_FrameProcData
{
  U32 frame_size;
  CV_FrameprocFlags flags;
};

typedef struct P2R_FrameProcNode P2R_FrameProcNode;
struct P2R_FrameProcNode
{
  P2R_FrameProcNode *next;
  RADDBGIC_Symbol *key;
  P2R_FrameProcData data;
};

typedef struct P2R_FrameProcMap P2R_FrameProcMap;
struct P2R_FrameProcMap
{
  P2R_FrameProcNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 pair_count;
};

//- rjf: scopes

typedef struct P2R_ScopeNode P2R_ScopeNode;
struct P2R_ScopeNode
{
  P2R_ScopeNode *next;
  RADDBGIC_Scope *scope;
  RADDBGIC_Symbol *symbol;
};

//- rjf: known global map

typedef struct P2R_KnownGlobalNode P2R_KnownGlobalNode;
struct P2R_KnownGlobalNode
{
  P2R_KnownGlobalNode *next;
  String8 key_name;
  U64     key_voff;
  U64     hash;
};

typedef struct P2R_KnownGlobalSet P2R_KnownGlobalSet;
struct P2R_KnownGlobalSet
{
  P2R_KnownGlobalNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 global_count;
};

typedef struct P2R_CtxParams P2R_CtxParams;
struct P2R_CtxParams
{
  RADDBGI_Arch arch;
  PDB_TpiHashParsed *tpi_hash;
  CV_LeafParsed *tpi_leaf;
  PDB_CoffSectionArray *sections;
  U64 fwd_map_bucket_count;
  U64 frame_proc_map_bucket_count;
  U64 known_global_map_bucket_count;
  U64 link_name_map_bucket_count;
};

typedef struct P2R_TypesSymbolsParams P2R_TypesSymbolsParams;
struct P2R_TypesSymbolsParams
{
  CV_SymParsed *sym;
  CV_SymParsed **sym_for_unit;
  U64 unit_count;
};

typedef struct P2R_LinkNameNode P2R_LinkNameNode;
struct P2R_LinkNameNode
{
  P2R_LinkNameNode *next;
  U64 voff;
  String8 name;
};

typedef struct P2R_LinkNameMap P2R_LinkNameMap;
struct P2R_LinkNameMap
{
  P2R_LinkNameNode **buckets;
  U64 buckets_count;
  U64 bucket_collision_count;
  U64 link_name_count;
};

typedef struct P2R_Ctx P2R_Ctx;
struct P2R_Ctx
{
  Arena *arena;
  
  // INPUT data
  RADDBGI_Arch arch;
  U64 addr_size;
  PDB_TpiHashParsed *hash;
  CV_LeafParsed *leaf;
  COFF_SectionHeader *sections;
  U64 section_count;
  
  // OUTPUT data
  RADDBGIC_Root *root;
  
  // TEMPORARY STATE
  P2R_FwdMap fwd_map;
  P2R_TypeRev *member_revisit_first;
  P2R_TypeRev *member_revisit_last;
  P2R_TypeRev *enum_revisit_first;
  P2R_TypeRev *enum_revisit_last;
  P2R_FrameProcMap frame_proc_map;
  P2R_ScopeNode *scope_stack;
  P2R_ScopeNode *scope_node_free;
  P2R_KnownGlobalSet known_globals;
  P2R_LinkNameMap link_names;
};

////////////////////////////////
//~ Conversion Output Type

typedef struct P2R_Out P2R_Out;
struct P2R_Out
{
  B32 good_parse;
  RADDBGIC_Root *root;
  String8List dump;
  String8List errors;
};

////////////////////////////////
//~ rjf: Command Line -> Conversion Parameters

internal P2R_Params *p2r_params_from_cmd_line(Arena *arena, CmdLine *cmdline);

////////////////////////////////
//~ rjf: COFF => RADDBGI Canonical Conversions

internal RADDBGI_BinarySectionFlags raddbgi_binary_section_flags_from_coff_section_flags(COFF_SectionFlags flags);

////////////////////////////////
//~ rjf: CodeView => RADDBGI Canonical Conversions

internal RADDBGI_Arch         raddbgi_arch_from_cv_arch(CV_Arch arch);
internal RADDBGI_RegisterCode raddbgi_reg_code_from_cv_reg_code(RADDBGI_Arch arch, CV_Reg reg_code);
internal RADDBGI_Language     raddbgi_language_from_cv_language(CV_Language language);

////////////////////////////////
//~ rjf: Conversion Implementation Helpers

//- rjf: pdb conversion context creation
internal P2R_Ctx *p2r_ctx_alloc(P2R_CtxParams *params, RADDBGIC_Root *out_root);

//- rjf: pdb types and symbols
internal void p2r_types_and_symbols(P2R_Ctx *pdb_ctx, P2R_TypesSymbolsParams *params);

//- rjf: decoding helpers
internal U32 p2r_u32_from_numeric(P2R_Ctx *ctx, CV_NumericParsed *num);
internal COFF_SectionHeader* p2r_sec_header_from_sec_num(P2R_Ctx *ctx, U32 sec_num);

//- rjf: type info
//
// TODO(allen): explain the overarching pattern of PDB type info translation here
// 1. main passes (out of order necessity) & after
// 2. resolve forward
// 3. cons type info
// 4. "resolve itype"
// 5. equipping members & enumerates
// 6. equipping source coordinates

// type info construction passes
internal void           p2r_type_cons_main_passes(P2R_Ctx *ctx);
internal CV_TypeId      p2r_type_resolve_fwd(P2R_Ctx *ctx, CV_TypeId itype);
internal RADDBGIC_Type* p2r_type_resolve_itype(P2R_Ctx *ctx, CV_TypeId itype);
internal void           p2r_type_equip_members(P2R_Ctx *ctx, RADDBGIC_Type *owern_type, CV_TypeId field_itype);
internal void           p2r_type_equip_enumerates(P2R_Ctx *ctx, RADDBGIC_Type *owner_type, CV_TypeId field_itype);

// type info construction helpers
internal RADDBGIC_Type* p2r_type_cons_basic(P2R_Ctx *ctx, CV_TypeId itype);
internal RADDBGIC_Type* p2r_type_cons_leaf_record(P2R_Ctx *ctx, CV_TypeId itype);
internal RADDBGIC_Type* p2r_type_resolve_and_check(P2R_Ctx *ctx, CV_TypeId itype);
internal void       p2r_type_resolve_arglist(Arena *arena, RADDBGIC_TypeList *out,
                                             P2R_Ctx *ctx, CV_TypeId arglist_itype);

// type info resolution helpers
internal RADDBGIC_Type* p2r_type_from_name(P2R_Ctx *ctx, String8 name);

// type fwd map
internal void      p2r_type_fwd_map_set(Arena *arena, P2R_FwdMap *map,
                                        CV_TypeId key, CV_TypeId val);
internal CV_TypeId p2r_type_fwd_map_get(P2R_FwdMap *map, CV_TypeId key);

//- rjf: symbol info

// symbol info construction
internal U64 p2r_hash_from_local_user_id(U64 sym_hash, U64 id);
internal U64 p2r_hash_from_scope_user_id(U64 sym_hash, U64 id);
internal U64 p2r_hash_from_symbol_user_id(U64 sym_hash, U64 id);
internal void p2r_symbol_cons(P2R_Ctx *ctx, CV_SymParsed *sym, U32 sym_unique_id);
internal void p2r_gather_link_names(P2R_Ctx *ctx, CV_SymParsed *sym);

// "frameproc" map
internal void                   p2r_symbol_frame_proc_write(P2R_Ctx *ctx,RADDBGIC_Symbol *key,
                                                            P2R_FrameProcData *data);
internal P2R_FrameProcData* p2r_symbol_frame_proc_read(P2R_Ctx *ctx, RADDBGIC_Symbol *key);

// scope stack
internal void p2r_symbol_push_scope(P2R_Ctx *ctx, RADDBGIC_Scope *scope, RADDBGIC_Symbol *symbol);
internal void p2r_symbol_pop_scope(P2R_Ctx *ctx);
internal void p2r_symbol_clear_scope_stack(P2R_Ctx *ctx);

#define p2r_symbol_current_scope(ctx)  ((ctx)->scope_stack == 0)?0:((ctx)->scope_stack->scope)
#define p2r_symbol_current_symbol(ctx) ((ctx)->scope_stack == 0)?0:((ctx)->scope_stack->symbol)

// PDB/C++ name parsing helper
internal U64 p2r_end_of_cplusplus_container_name(String8 str);

// global deduplication
internal U64  p2r_known_global_hash(String8 name, U64 voff);

internal B32  p2r_known_global_lookup(P2R_KnownGlobalSet *set, String8 name, U64 voff);
internal void p2r_known_global_insert(Arena *arena, P2R_KnownGlobalSet *set,
                                      String8 name, U64 voff);


// location info helpers
internal RADDBGIC_Location* p2r_location_from_addr_reg_off(P2R_Ctx *ctx,
                                                           RADDBGI_RegisterCode reg_code,
                                                           U32 reg_byte_size,
                                                           U32 reg_byte_pos,
                                                           S64 offset,
                                                           B32 extra_indirection);

internal CV_EncodedFramePtrReg p2r_cv_encoded_fp_reg_from_proc(P2R_Ctx *ctx,
                                                               RADDBGIC_Symbol *proc,
                                                               B32 param_base);

internal RADDBGI_RegisterCode p2r_reg_code_from_arch_encoded_fp_reg(RADDBGI_Arch arch,
                                                                    CV_EncodedFramePtrReg encoded_reg);

internal void p2r_location_over_lvar_addr_range(P2R_Ctx *ctx,
                                                RADDBGIC_LocationSet *locset,
                                                RADDBGIC_Location *location,
                                                CV_LvarAddrRange *range,
                                                CV_LvarAddrGap *gaps, U64 gap_count);

// link names
internal void    p2r_link_name_save(Arena *arena, P2R_LinkNameMap *map,
                                    U64 voff, String8 name);
internal String8 p2r_link_name_find(P2R_LinkNameMap *map, U64 voff);

////////////////////////////////
//~ rjf: Top-Level Conversion Entry Point

internal P2R_Out *p2r_convert(Arena *arena, P2R_Params *params);

#endif // RADDBGI_FROM_PDB_H
