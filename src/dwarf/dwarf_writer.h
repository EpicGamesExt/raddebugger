// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_WRITER_H
#define DWARF_WRITER_H

////////////////////////////////

typedef enum
{
  DW_IntEnc_Null,
  DW_IntEnc_1Byte,
  DW_IntEnc_2Byte,
  DW_IntEnc_4Byte,
  DW_IntEnc_LEB128,
} DW_IntEnc;

////////////////////////////////

typedef enum
{
  DW_WriterFormKind_Null,
  DW_WriterFormKind_Flag,
  DW_WriterFormKind_SInt,
  DW_WriterFormKind_UInt,
  DW_WriterFormKind_Address,
  DW_WriterFormKind_Ref,
  DW_WriterFormKind_LinePtr,
  DW_WriterFormKind_MacPtr,
  DW_WriterFormKind_RngListPtr,
  DW_WriterFormKind_Data,
  DW_WriterFormKind_Block,
  DW_WriterFormKind_String,
  DW_WriterFormKind_ExprLoc,
  DW_WriterFormKind_Implicit,
} DW_WriterFormKind;

typedef struct DW_WriterForm
{
  DW_WriterFormKind kind;
  union {
    B8      flag;
    S64     sint;
    U64     uint;
    U64     address;
    S64     implicit;
    union {
      struct {
        struct DW_WriterTag *ref;
        U64                  line_ptr;
        void                *mac_ptr;
        void                *rng_list_ptr;
      };
      struct {
        U64 ref_offset;
        U64 line_offset;
        U64 mac_ptr_offset;
        U64 rng_list_offset;
      };
    };
    String8 data;
    String8 block;
    String8 string;
    String8 exprloc;
  };
} DW_WriterForm;

typedef struct DW_WriterXForm
{
  DW_WriterForm writer;
  DW_Form       reader;
} DW_WriterXForm;

typedef struct DW_WriterAttrib
{
  DW_AttribKindEnum kind;
  DW_WriterXForm    form;
  struct DW_WriterAttrib *next;
} DW_WriterAttrib;

typedef struct DW_WriterTag
{
  DW_TagKindEnum       kind;
  struct DW_WriterTag *next;
  struct DW_WriterTag *parent;
  struct DW_WriterTag *first_child;
  struct DW_WriterTag *last_child;
  U64                  attrib_count;
  U64                  abbrev_id;
  U64                  info_off;
  DW_WriterAttrib     *first_attrib;
  DW_WriterAttrib     *last_attrib;
} DW_WriterTag;

typedef struct DW_WriterTagChunk
{
  U64                       count;
  U64                       max;
  DW_WriterTag             *v;
  struct DW_WriterTagChunk *next;
} DW_WriterTagChunk;

typedef struct DW_WriterTagChunkList
{
  U64 chunk_count;
  U64 total_count;
  DW_WriterTagChunk *first;
  DW_WriterTagChunk *last;
} DW_WriterTagChunkList;

typedef struct DW_WriterAttribChunk
{
  U64 count;
  U64 max;
  DW_WriterAttrib *v;
  struct DW_WriterAttribChunk *next;
} DW_WriterAttribChunk;

typedef struct DW_WriterAttribChunkList
{
  U64 chunk_count;
  U64 total_count;
  DW_WriterAttribChunk *first;
  DW_WriterAttribChunk *last;
} DW_WriterAttribChunkList;

typedef struct DW_WriterLine
{
  struct DW_WriterLine *next;
} DW_WriterLine;

typedef struct DW_WriterFile
{
  struct DW_WriterFile *next;
  String8 path;
  U64     time_stamp;
  U64     size;
  String8 md5;
  String8 source;
  // computed during line table emit step
  U64     file_idx;
  U64     dir_idx;
} DW_WriterFile;

typedef struct DW_LineInst
{
  DW_StdOpcode opcode;
  union {
    U64            advance_pc;
    S64            advance_line;
    DW_WriterFile *set_file;
    U64            set_column;
    U16            fixed_advance_pc;
    U64            set_isa;
    struct {
      DW_ExtOpcode ext;
      U64          set_address;
      U64          set_discriminator;
    };
  };
} DW_LineInst;

typedef struct DW_LineInstNode
{
  DW_LineInst v;
  struct DW_LineInstNode *next;
} DW_LineInstNode;

typedef struct DW_LineInstList
{
  U64            count;
  DW_LineInstNode *first;
  DW_LineInstNode *last;
} DW_LineInstList;

typedef struct DW_WriterFixup
{
  DW_WriterTag *tag;
  U8           *ptr;
} DW_WriterFixup;

typedef struct DW_WriterFixupNode
{
  DW_WriterFixup             v;
  struct DW_WriterFixupNode *next;
} DW_WriterFixupNode;

typedef struct DW_WriterFixupList
{
  U64                 count;
  DW_WriterFixupNode *first;
  DW_WriterFixupNode *last;
} DW_WriterFixupList;

typedef struct DW_WriterSection
{
  void       *length;
  String8List srl;
} DW_WriterSection;

typedef struct DW_Writer
{
  Arena                   *arena;

  // Compile Unit
  Arch                     arch;
  DW_Version               version;
  DW_Format                format;
  DW_CompUnitKind          cu_kind;
  U8                       address_size;
  U8                       segsel_size;

  // Abbrev
  U64                      abbrev_base_info_off;
  HashTable               *abbrev_id_map;

  // Info
  DW_WriterTag            *root;
  DW_WriterTag            *current;

  // Line
  struct {
    U8              min_inst_len;
    U8              max_ops_per_inst;
    U8              default_is_stmt;
    S8              line_base;
    U8              line_range;
    U8              opcode_base;
    DW_LineInstList line_insts;

    DW_WriterFile *file;
    U64            ln;
    U64            col;
    U64            addr;

    U64            file_count;
    DW_WriterFile *first_file;
    DW_WriterFile *last_file;
  } line;

  // Emit
  DW_WriterFixupList       fixups;
  DW_WriterSection         sections[DW_Section_Count];
  DW_WriterAttribChunkList attrib_chunk_list;
  DW_WriterTagChunkList    tag_chunk_list;
} DW_Writer;

////////////////////////////////
// Writer

internal DW_Writer * dw_writer_begin(DW_Format format, DW_Version version, DW_CompUnitKind cu_kind, Arch arch);
internal void        dw_writer_end  (DW_Writer **writer_ptr);

////////////////////////////////
// Form

internal U64 dw_serial_push_form(Arena *arena, String8List *srl, DW_Version version, DW_Format format, U8 address_size, DW_WriterFixupList *fixups, DW_WriterXForm form);

////////////////////////////////
// Info

internal DW_WriterTag * dw_writer_tag_begin(DW_Writer *writer, DW_TagKind kind);
internal void           dw_writer_tag_end  (DW_Writer *writer);

internal DW_WriterAttrib * dw_writer_push_attrib             (DW_Writer *writer, DW_AttribKind kind, DW_WriterForm form        );
internal DW_WriterAttrib * dw_writer_push_attrib_address     (DW_Writer *writer, DW_AttribKind kind, U64           address     );
internal DW_WriterAttrib * dw_writer_push_attrib_block       (DW_Writer *writer, DW_AttribKind kind, String8       block       );
internal DW_WriterAttrib * dw_writer_push_attrib_data        (DW_Writer *writer, DW_AttribKind kind, String8       data        );
internal DW_WriterAttrib * dw_writer_push_attrib_string      (DW_Writer *writer, DW_AttribKind kind, String8       string      );
internal DW_WriterAttrib * dw_writer_push_attrib_stringf     (DW_Writer *writer, DW_AttribKind kind, char         *fmt, ...    );
internal DW_WriterAttrib * dw_writer_push_attrib_flag        (DW_Writer *writer, DW_AttribKind kind, B8            flag        );
internal DW_WriterAttrib * dw_writer_push_attrib_sint        (DW_Writer *writer, DW_AttribKind kind, S64           sint        );
internal DW_WriterAttrib * dw_writer_push_attrib_uint        (DW_Writer *writer, DW_AttribKind kind, U64           uint        );
internal DW_WriterAttrib * dw_writer_push_attrib_enum        (DW_Writer *writer, DW_AttribKind kind, S64           e           );
internal DW_WriterAttrib * dw_writer_push_attrib_ref         (DW_Writer *writer, DW_AttribKind kind, DW_WriterTag *ref         );
internal DW_WriterAttrib * dw_writer_push_attrib_exprloc     (DW_Writer *writer, DW_AttribKind kind, String8       exprloc     );
internal DW_WriterAttrib * dw_writer_push_attrib_expression  (DW_Writer *writer, DW_AttribKind kind, DW_ExprEnc *encs, U64 encs_count);
internal DW_WriterAttrib * dw_writer_push_attrib_line_ptr    (DW_Writer *writer, DW_AttribKind kind, U64           line_ptr    );
internal DW_WriterAttrib * dw_writer_push_attrib_mac_ptr     (DW_Writer *writer, DW_AttribKind kind, void *        mac_ptr     );
internal DW_WriterAttrib * dw_writer_push_attrib_rng_list_ptr(DW_Writer *writer, DW_AttribKind kind, void *        rng_list_ptr);
internal DW_WriterAttrib * dw_writer_push_attrib_implicit    (DW_Writer *writer, DW_AttribKind kind, S64           implicit    );
#define dw_writer_push_attrib_expressionv(w, k, ...) dw_writer_push_attrib_expression(w, k, (DW_ExprEnc[]){ __VA_ARGS__ }, ArrayCount(((DW_ExprEnc[]){ __VA_ARGS__ })) )

////////////////////////////////
// Line

internal void              dw_line_inst_list_push_node(DW_LineInstList *list, DW_LineInstNode *node);
internal DW_LineInstNode * dw_line_inst_list_push     (Arena *arena, DW_LineInstList *list, DW_LineInst op);

// std opcodes
#define DW_LNS_copy()               (DW_LineInst){ .opcode = DW_StdOpcode_Copy                                  }
#define DW_LNS_advance_pc(d)        (DW_LineInst){ .opcode = DW_StdOpcode_AdvancePc,      .advance_pc   = d     }
#define DW_LNS_advance_line(s)      (DW_LineInst){ .opcode = DW_StdOpcode_AdvanceLine,    .advance_line = s     }
#define DW_LNS_set_file(f)          (DW_LineInst){ .opcode = DW_StdOpcode_SetFile,        .set_file     = f     }
#define DW_LNS_set_column(c)        (DW_LineInst){ .opcode = DW_StdOpcode_SetColumn,      .set_column   = c     }
#define DW_LNS_negate_stmt()        (DW_LineInst){ .opcode = DW_StdOpcode_NegateStmt                            }
#define DW_LNS_set_basic_block()    (DW_LineInst){ .opcode = DW_StdOpcode_SetBasicBlock                         }
#define DW_LNS_const_add_pc(a)      (DW_LineInst){ .opcode = DW_StdOpcode_ConstAddPc                            }
#define DW_LNS_fixed_advance_pc(a)  (DW_LineInst){ .opcode = DW_StdOpcode_FixedAdvancePc, .fixed_advance_pc = a }
#define DW_LNS_set_prologue_end()   (DW_LineInst){ .opcode = DW_StdOpcode_SetPrologueEnd                        }
#define DW_LNS_set_epilogue_begin() (DW_LineInst){ .opcode = DW_StdOpcode_SetEpilogueBegin                      }
#define DW_LNS_set_isa(i)           (DW_LineInst){ .opcode = DW_StdOpcode_SetIsa,          .set_isa = i         }

// ext opcodes
#define DW_LNE_end_sequence()       (DW_LineInst){ .opcode = DW_StdOpcode_ExtendedOpcode, .ext = DW_ExtOpcode_EndSequence                              }
#define DW_LNE_set_address(a)       (DW_LineInst){ .opcode = DW_StdOpcode_ExtendedOpcode, .ext = DW_ExtOpcode_SetAddress,       .set_address = a       }
#define DW_LNE_set_discriminator(d) (DW_LineInst){ .opcode = DW_StdOpcode_ExtendedOpcode, .ext = DW_ExtOpcode_SetDiscriminator, .set_discriminator = d }

internal void            dw_writer_line_emit            (DW_Writer *writer, DW_WriterFile *file, U64 ln, U64 col, U64 addr);
internal void            dw_writer_line_set_address     (DW_Writer *writer, U64 address);
internal void            dw_writer_line_set_prologue_end(DW_Writer *writer);
internal void            dw_writer_line_epilogue_begin  (DW_Writer *writer);
internal void            dw_writer_line_set_isa         (DW_Writer *writer, U64 isa);
internal DW_WriterFile * dw_writer_new_file             (DW_Writer *writer, String8 path);

////////////////////////////////
// Emit

internal void dw_writer_emit(DW_Writer *writer);
#ifdef OBJ_H
internal void dw_writer_emit_to_obj(DW_Writer *writer, OBJ *obj);
#endif

////////////////////////////////
// Format Helpers

internal DW_IntEnc dw_int_enc_from_sint(S64 v);
internal DW_IntEnc dw_int_enc_from_uint(U64 v);

internal U64 dw_size_from_sint(S64 v);
internal U64 dw_size_from_uint(U64 v);

#endif // DWARF_WRITER_H

