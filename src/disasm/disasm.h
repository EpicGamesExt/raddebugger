// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DISASM_H
#define DISASM_H

////////////////////////////////
//~ rjf: Disassembly Syntax Types

typedef enum DASM_Syntax
{
  DASM_Syntax_Intel,
  DASM_Syntax_ATT,
  DASM_Syntax_COUNT
}
DASM_Syntax;

////////////////////////////////
//~ rjf: Disassembly Instruction Info Types

typedef U32 DASM_InstFlags;
enum
{
  DASM_InstFlag_Call                        = (1<<0),
  DASM_InstFlag_Branch                      = (1<<1),
  DASM_InstFlag_UnconditionalJump           = (1<<2),
  DASM_InstFlag_Return                      = (1<<3),
  DASM_InstFlag_NonFlow                     = (1<<4),
  DASM_InstFlag_Repeats                     = (1<<5),
  DASM_InstFlag_ChangesStackPointer         = (1<<6),
  DASM_InstFlag_ChangesStackPointerVariably = (1<<7),
};

typedef struct DASM_Inst DASM_Inst;
struct DASM_Inst
{
  DASM_InstFlags flags;
  U32 size;
  String8 string;
  U64 jump_dest_vaddr;
};

////////////////////////////////
//~ rjf: Control Flow Analysis Types

typedef struct DASM_CtrlFlowPoint DASM_CtrlFlowPoint;
struct DASM_CtrlFlowPoint
{
  U64 vaddr;
  U64 jump_dest_vaddr;
  DASM_InstFlags inst_flags;
};

typedef struct DASM_CtrlFlowPointNode DASM_CtrlFlowPointNode;
struct DASM_CtrlFlowPointNode
{
  DASM_CtrlFlowPointNode *next;
  DASM_CtrlFlowPoint v;
};

typedef struct DASM_CtrlFlowPointList DASM_CtrlFlowPointList;
struct DASM_CtrlFlowPointList
{
  DASM_CtrlFlowPointNode *first;
  DASM_CtrlFlowPointNode *last;
  U64 count;
};

typedef struct DASM_CtrlFlowInfo DASM_CtrlFlowInfo;
struct DASM_CtrlFlowInfo
{
  DASM_CtrlFlowPointList exit_points;
  U64 total_size;
};

////////////////////////////////
//~ rjf: Disassembly Text Decoration Types

typedef U32 DASM_StyleFlags;
enum
{
  DASM_StyleFlag_Addresses        = (1<<0),
  DASM_StyleFlag_CodeBytes        = (1<<1),
  DASM_StyleFlag_SourceFilesNames = (1<<2),
  DASM_StyleFlag_SourceLines      = (1<<3),
  DASM_StyleFlag_SymbolNames      = (1<<4),
};

////////////////////////////////
//~ rjf: Disassembling Parameters Bundle

typedef struct DASM_Params DASM_Params;
struct DASM_Params
{
  U64 vaddr;
  Arch arch;
  DASM_StyleFlags style_flags;
  DASM_Syntax syntax;
  U64 base_vaddr;
  DI_Key dbgi_key;
};

////////////////////////////////
//~ rjf: Disassembly Request Bundle

typedef struct DASM_Request DASM_Request;
struct DASM_Request
{
  C_Root root;
  U128 hash;
  DASM_Params params;
};

typedef struct DASM_RequestNode DASM_RequestNode;
struct DASM_RequestNode
{
  DASM_RequestNode *next;
  DASM_Request v;
};

////////////////////////////////
//~ rjf: Disassembly Text Line Types

typedef U32 DASM_LineFlags;
enum
{
  DASM_LineFlag_Decorative = (1<<0),
};

typedef struct DASM_Line DASM_Line;
struct DASM_Line
{
  U32 code_off;
  DASM_LineFlags flags;
  U64 addr;
  Rng1U64 text_range;
};

typedef struct DASM_LineChunkNode DASM_LineChunkNode;
struct DASM_LineChunkNode
{
  DASM_LineChunkNode *next;
  DASM_Line *v;
  U64 cap;
  U64 count;
};

typedef struct DASM_LineChunkList DASM_LineChunkList;
struct DASM_LineChunkList
{
  DASM_LineChunkNode *first;
  DASM_LineChunkNode *last;
  U64 node_count;
  U64 line_count;
};

typedef struct DASM_LineArray DASM_LineArray;
struct DASM_LineArray
{
  DASM_Line *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Value Bundle Type

typedef struct DASM_Info DASM_Info;
struct DASM_Info
{
  C_Key text_key;
  DASM_LineArray lines;
};

////////////////////////////////
//~ rjf: Instruction Decoding/Disassembling Type Functions

internal DASM_Inst dasm_inst_from_code(Arena *arena, Arch arch, U64 vaddr, String8 code, DASM_Syntax syntax);

////////////////////////////////
//~ rjf: Control Flow Analysis

internal DASM_CtrlFlowInfo dasm_ctrl_flow_info_from_arch_vaddr_code(Arena *arena, DASM_InstFlags exit_points_mask, Arch arch, U64 vaddr, String8 code);

////////////////////////////////
//~ rjf: Parameter Type Functions

internal B32 dasm_params_match(DASM_Params *a, DASM_Params *b);

////////////////////////////////
//~ rjf: Line Type Functions

internal void dasm_line_chunk_list_push(Arena *arena, DASM_LineChunkList *list, U64 cap, DASM_Line *line);
internal DASM_LineArray dasm_line_array_from_chunk_list(Arena *arena, DASM_LineChunkList *list);
internal U64 dasm_line_array_idx_from_code_off__linear_scan(DASM_LineArray *array, U64 off);
internal U64 dasm_line_array_code_off_from_idx(DASM_LineArray *array, U64 idx);

////////////////////////////////
//~ rjf: Artifact Cache Hooks / Lookups

internal AC_Artifact dasm_artifact_create(String8 key, B32 *cancel_signal, B32 *retry_out);
internal void dasm_artifact_destroy(AC_Artifact artifact);
internal DASM_Info dasm_info_from_hash_params(Access *access, U128 hash, DASM_Params *params);
internal DASM_Info dasm_info_from_key_params(Access *access, C_Key key, DASM_Params *params, U128 *hash_out);

#endif // DISASM_H
