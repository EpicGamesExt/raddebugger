// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef RDI_BREAKPAD_FROM_PDB_H
#define RDI_BREAKPAD_FROM_PDB_H

////////////////////////////////
//~ rjf: Baking Tasks

//- rjf: unit vmap baking

typedef struct P2B_BakeUnitVMapIn P2B_BakeUnitVMapIn;
struct P2B_BakeUnitVMapIn
{
  RDIM_UnitChunkList *units;
};
ASYNC_WORK_DEF(p2b_bake_unit_vmap_work);

//- rjf: line table baking

typedef struct P2B_BakeLineTablesIn P2B_BakeLineTablesIn;
struct P2B_BakeLineTablesIn
{
  RDIM_LineTableChunkList *line_tables;
};
ASYNC_WORK_DEF(p2b_bake_line_table_work);

//- rjf: per-procedure chunk dumping

typedef struct P2B_DumpProcChunkIn P2B_DumpProcChunkIn;
struct P2B_DumpProcChunkIn
{
  RDI_VMapEntry *unit_vmap;
  U32 unit_vmap_count;
  U32 *unit_line_table_idxs;
  U64 unit_count;
  RDIM_LineTableBakeResult *line_tables_bake;
  RDIM_SymbolChunkNode *chunk;
};
ASYNC_WORK_DEF(p2b_dump_proc_chunk_work);

////////////////////////////////
//~ rjf: Globals

global ASYNC_Root *p2b_async_root = 0;

#endif // RDI_BREAKPAD_FROM_PDB_H
