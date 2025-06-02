// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDIM_MAKE_HELP
#define RDIM_MAKE_HELP

//- rjf: line table baking task types

typedef struct RDIM_BakeLineTablesIn RDIM_BakeLineTablesIn;
struct RDIM_BakeLineTablesIn
{
  RDIM_LineTableChunkList *line_tables;
};

//- rjf: string map baking task types

typedef struct RDIM_BakeSrcFilesStringsIn RDIM_BakeSrcFilesStringsIn;
struct RDIM_BakeSrcFilesStringsIn
{
  RDIM_BakeStringMapTopology *top;
  RDIM_BakeStringMapLoose **maps;
  RDIM_SrcFileChunkList *list;
};

typedef struct RDIM_BakeUnitsStringsIn RDIM_BakeUnitsStringsIn;
struct RDIM_BakeUnitsStringsIn
{
  RDIM_BakeStringMapTopology *top;
  RDIM_BakeStringMapLoose **maps;
  RDIM_UnitChunkList *list;
};

typedef struct RDIM_BakeUDTsStringsInNode RDIM_BakeUDTsStringsInNode;
struct RDIM_BakeUDTsStringsInNode
{
  RDIM_BakeUDTsStringsInNode *next;
  RDIM_UDT *v;
  RDI_U64 count;
};

typedef struct RDIM_BakeTypesStringsInNode RDIM_BakeTypesStringsInNode;
struct RDIM_BakeTypesStringsInNode
{
  RDIM_BakeTypesStringsInNode *next;
  RDIM_Type *v;
  RDI_U64 count;
};

typedef struct RDIM_BakeTypesStringsIn RDIM_BakeTypesStringsIn;
struct RDIM_BakeTypesStringsIn
{
  RDIM_BakeStringMapTopology *top;
  RDIM_BakeStringMapLoose **maps;
  RDIM_BakeTypesStringsInNode *first;
  RDIM_BakeTypesStringsInNode *last;
};

typedef struct RDIM_BakeUDTsStringsIn RDIM_BakeUDTsStringsIn;
struct RDIM_BakeUDTsStringsIn
{
  RDIM_BakeStringMapTopology *top;
  RDIM_BakeStringMapLoose **maps;
  RDIM_BakeUDTsStringsInNode *first;
  RDIM_BakeUDTsStringsInNode *last;
};

typedef struct RDIM_BakeSymbolsStringsInNode RDIM_BakeSymbolsStringsInNode;
struct RDIM_BakeSymbolsStringsInNode
{
  RDIM_BakeSymbolsStringsInNode *next;
  RDIM_Symbol *v;
  RDI_U64 count;
};

typedef struct RDIM_BakeSymbolsStringsIn RDIM_BakeSymbolsStringsIn;
struct RDIM_BakeSymbolsStringsIn
{
  RDIM_BakeStringMapTopology *top;
  RDIM_BakeStringMapLoose **maps;
  RDIM_BakeSymbolsStringsInNode *first;
  RDIM_BakeSymbolsStringsInNode *last;
};

typedef struct RDIM_BakeInlineSiteStringsInNode RDIM_BakeInlineSiteStringsInNode;
struct RDIM_BakeInlineSiteStringsInNode
{
  RDIM_BakeInlineSiteStringsInNode *next;
  RDIM_InlineSite *v;
  RDI_U64 count;
};

typedef struct RDIM_BakeInlineSiteStringsIn RDIM_BakeInlineSiteStringsIn;
struct RDIM_BakeInlineSiteStringsIn
{
  RDIM_BakeStringMapTopology *top;
  RDIM_BakeStringMapLoose **maps;
  RDIM_BakeInlineSiteStringsInNode *first;
  RDIM_BakeInlineSiteStringsInNode *last;
};

typedef struct RDIM_BakeScopesStringsInNode RDIM_BakeScopesStringsInNode;
struct RDIM_BakeScopesStringsInNode
{
  RDIM_BakeScopesStringsInNode *next;
  RDIM_Scope *v;
  RDI_U64 count;
};

typedef struct RDIM_BakeScopesStringsIn RDIM_BakeScopesStringsIn;
struct RDIM_BakeScopesStringsIn
{
  RDIM_BakeStringMapTopology *top;
  RDIM_BakeStringMapLoose **maps;
  RDIM_BakeScopesStringsInNode *first;
  RDIM_BakeScopesStringsInNode *last;
};

//- rjf: OLD string map baking types

typedef struct RDIM_BuildBakeStringMapIn RDIM_BuildBakeStringMapIn;
struct RDIM_BuildBakeStringMapIn
{
  RDIM_BakePathTree *path_tree;
  RDIM_BakeParams *params;
};

typedef struct RDIM_BuildBakeNameMapIn RDIM_BuildBakeNameMapIn;
struct RDIM_BuildBakeNameMapIn
{
  RDI_NameMapKind k;
  RDI_U64 *type_indices;
  RDIM_BakeParams *params;
};

//- rjf: string map joining task types

typedef struct RDIM_JoinBakeStringMapSlotsIn RDIM_JoinBakeStringMapSlotsIn;
struct RDIM_JoinBakeStringMapSlotsIn
{
  RDIM_BakeStringMapTopology *top;
  RDIM_BakeStringMapLoose **src_maps;
  U64 src_maps_count;
  RDIM_BakeStringMapLoose *dst_map;
  Rng1U64 slot_idx_range;
};

//- rjf: string map sorting task types

typedef struct RDIM_SortBakeStringMapSlotsIn RDIM_SortBakeStringMapSlotsIn;
struct RDIM_SortBakeStringMapSlotsIn
{
  RDIM_BakeStringMapTopology *top;
  RDIM_BakeStringMapLoose *src_map;
  RDIM_BakeStringMapLoose *dst_map;
  U64 slot_idx;
  U64 slot_count;
};

//- rjf: debug info baking task types

typedef struct RDIM_BakeUnitsIn RDIM_BakeUnitsIn;
struct RDIM_BakeUnitsIn
{
  RDIM_BakeStringMapTight *strings;
  RDIM_BakePathTree *path_tree;
  RDIM_UnitChunkList *units;
};

typedef struct RDIM_BakeUnitVMapIn RDIM_BakeUnitVMapIn;
struct RDIM_BakeUnitVMapIn
{
  RDIM_UnitChunkList *units;
};

typedef struct RDIM_BakeSrcFilesIn RDIM_BakeSrcFilesIn;
struct RDIM_BakeSrcFilesIn
{
  RDIM_BakeStringMapTight *strings;
  RDIM_BakePathTree *path_tree;
  RDIM_SrcFileChunkList *src_files;
};

typedef struct RDIM_BakeUDTsIn RDIM_BakeUDTsIn;
struct RDIM_BakeUDTsIn
{
  RDIM_BakeStringMapTight *strings;
  RDIM_UDTChunkList *udts;
  RDI_U64 *type_indices;
};

typedef struct RDIM_BakeGlobalVariablesIn RDIM_BakeGlobalVariablesIn;
struct RDIM_BakeGlobalVariablesIn
{
  RDIM_BakeStringMapTight *strings;
  RDIM_SymbolChunkList *global_variables;
  RDI_U64 *type_indices;
};

typedef struct RDIM_BakeGlobalVMapIn RDIM_BakeGlobalVMapIn;
struct RDIM_BakeGlobalVMapIn
{
  RDIM_SymbolChunkList *global_variables;
};

typedef struct RDIM_BakeThreadVariablesIn RDIM_BakeThreadVariablesIn;
struct RDIM_BakeThreadVariablesIn
{
  RDIM_BakeStringMapTight *strings;
  RDIM_SymbolChunkList *thread_variables;
  RDI_U64 *type_indices;
};

typedef struct RDIM_BakeProceduresIn RDIM_BakeProceduresIn;
struct RDIM_BakeProceduresIn
{
  RDIM_BakeStringMapTight *strings;
  RDIM_SymbolChunkList *procedures;
  RDI_U64 *type_indices;
  RDIM_String8List *location_blocks;
  RDIM_String8List *location_data_blobs;
};

typedef struct RDIM_BakeScopesIn RDIM_BakeScopesIn;
struct RDIM_BakeScopesIn
{
  RDIM_BakeStringMapTight *strings;
  RDIM_ScopeChunkList *scopes;
  RDI_U64 *type_indices;
  RDIM_String8List *location_blocks;
  RDIM_String8List *location_data_blobs;
};

typedef struct RDIM_BakeScopeVMapIn RDIM_BakeScopeVMapIn;
struct RDIM_BakeScopeVMapIn
{
  RDIM_ScopeChunkList *scopes;
};

typedef struct RDIM_BakeInlineSitesIn RDIM_BakeInlineSitesIn;
struct RDIM_BakeInlineSitesIn
{
  RDIM_BakeStringMapTight *strings;
  RDIM_InlineSiteChunkList *inline_sites;
  RDI_U64 *type_indices;
};

typedef struct RDIM_BakeFilePathsIn RDIM_BakeFilePathsIn;
struct RDIM_BakeFilePathsIn
{
  RDIM_BakeStringMapTight *strings;
  RDIM_BakePathTree *path_tree;
};

typedef struct RDIM_BakeStringsIn RDIM_BakeStringsIn;
struct RDIM_BakeStringsIn
{
  RDIM_BakeStringMapTight *strings;
};

typedef struct RDIM_BakeTypeNodesIn RDIM_BakeTypeNodesIn;
struct RDIM_BakeTypeNodesIn
{
  RDIM_BakeStringMapTight *strings;
  RDIM_BakeIdxRunMap *idx_runs;
  RDIM_TypeChunkList *types;
  RDI_U64 *type_indices;
};

typedef struct RDIM_BakeNameMapIn RDIM_BakeNameMapIn;
struct RDIM_BakeNameMapIn
{
  RDIM_BakeStringMapTight *strings;
  RDIM_BakeIdxRunMap *idx_runs;
  RDIM_BakeNameMap *map;
  RDI_NameMapKind kind;
};

typedef struct RDIM_BakeIdxRunsIn RDIM_BakeIdxRunsIn;
struct RDIM_BakeIdxRunsIn
{
  RDIM_BakeIdxRunMap *idx_runs;
};

////////////////////////////////
//~ rjf: Baking Stage Tasks

//- rjf: unsorted bake string map building
ASYNC_WORK_DEF(p2r_bake_src_files_strings_work);
ASYNC_WORK_DEF(p2r_bake_units_strings_work);
ASYNC_WORK_DEF(p2r_bake_types_strings_work);
ASYNC_WORK_DEF(p2r_bake_udts_strings_work);
ASYNC_WORK_DEF(p2r_bake_symbols_strings_work);
ASYNC_WORK_DEF(p2r_bake_scopes_strings_work);
ASYNC_WORK_DEF(p2r_bake_line_tables_work);

//- rjf: bake string map joining
ASYNC_WORK_DEF(p2r_bake_string_map_join_work);

//- rjf: bake string map sorting
ASYNC_WORK_DEF(p2r_bake_string_map_sort_work);

//- rjf: pass 1: interner/deduper map builds
ASYNC_WORK_DEF(p2r_build_bake_name_map_work);

//- rjf: pass 2: string-map-dependent debug info stream builds
ASYNC_WORK_DEF(p2r_bake_units_work);
ASYNC_WORK_DEF(p2r_bake_unit_vmap_work);
ASYNC_WORK_DEF(p2r_bake_src_files_work);
ASYNC_WORK_DEF(p2r_bake_udts_work);
ASYNC_WORK_DEF(p2r_bake_global_variables_work);
ASYNC_WORK_DEF(p2r_bake_global_vmap_work);
ASYNC_WORK_DEF(p2r_bake_thread_variables_work);
ASYNC_WORK_DEF(p2r_bake_procedures_work);
ASYNC_WORK_DEF(p2r_bake_scopes_work);
ASYNC_WORK_DEF(p2r_bake_scope_vmap_work);
ASYNC_WORK_DEF(p2r_bake_file_paths_work);
ASYNC_WORK_DEF(p2r_bake_strings_work);

//- rjf: pass 3: idx-run-map-dependent debug info stream builds
ASYNC_WORK_DEF(p2r_bake_type_nodes_work);
ASYNC_WORK_DEF(p2r_bake_name_map_work);
ASYNC_WORK_DEF(p2r_bake_idx_runs_work);

typedef struct RDIM_HelpState RDIM_HelpState;
struct RDIM_HelpState
{
  Arena *arena;
  U64 work_thread_arenas_count;
  Arena **work_thread_arenas;
};

////////////////////////////////

global RDIM_HelpState *rdim_help_state = 0;

////////////////////////////////

internal RDIM_HelpState *             rdim_help_init(void);
internal RDIM_BakeResults             rdim_bake(RDIM_HelpState *state, RDIM_BakeParams *in);
internal RDIM_SerializedSectionBundle rdim_compress(Arena *arena, RDIM_SerializedSectionBundle *in);

#endif // RDIM_MAKE_HELP
