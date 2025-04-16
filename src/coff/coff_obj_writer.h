#ifndef COFF_OBJ_WRITER_H
#define COFF_OBJ_WRITER_H

typedef enum
{
  COFF_SymbolLocation_Null,
  COFF_SymbolLocation_Section,
  COFF_SymbolLocation_Abs,
  COFF_SymbolLocation_Undef
} COFF_SymbolLocationType;

typedef struct COFF_SymbolLocation
{
  COFF_SymbolLocationType type;
  union {
    struct COFF_ObjSection *section;
  } u;
} COFF_SymbolLocation;

typedef struct COFF_ObjSymbol
{
  String8                 name;
  U32                     value;
  COFF_SymbolLocation     loc;
  COFF_SymbolType         type;
  COFF_SymStorageClass    storage_class;
  String8List             aux_symbols;
  U32                     idx;
} COFF_ObjSymbol;

typedef struct COFF_ObjSymbolNode
{
  struct COFF_ObjSymbolNode *next;
  COFF_ObjSymbol             v;
} COFF_ObjSymbolNode;

typedef struct COFF_ObjReloc
{
  U32             apply_off;
  COFF_ObjSymbol *symbol;
  COFF_RelocType  type;
} COFF_ObjReloc;

typedef struct COFF_ObjRelocNode
{
  struct COFF_ObjRelocNode *next;
  COFF_ObjReloc             v;
} COFF_ObjRelocNode;

typedef struct COFF_ObjSection
{
  String8           name;
  String8List       data;
  COFF_SectionFlags flags;

  U64                reloc_count;
  COFF_ObjRelocNode *reloc_first;
  COFF_ObjRelocNode *reloc_last;

  U32 section_number;
} COFF_ObjSection;

typedef struct COFF_ObjSectionNode
{
  struct COFF_ObjSectionNode *next;
  COFF_ObjSection             v;
} COFF_ObjSectionNode;

typedef struct COFF_ObjWriter
{
  Arena           *arena;
  COFF_TimeStamp   time_stamp;
  COFF_MachineType machine_type;

  U64                 symbol_count;
  COFF_ObjSymbolNode *symbol_first;
  COFF_ObjSymbolNode *symbol_last;

  U64                  sect_count;
  COFF_ObjSectionNode *sect_first;
  COFF_ObjSectionNode *sect_last;
} COFF_ObjWriter;

////////////////////////////////

internal COFF_ObjWriter*  coff_obj_writer_alloc(COFF_TimeStamp time_stamp, COFF_MachineType machine_type);
internal void             coff_obj_writer_release(COFF_ObjWriter **obj_writer);
internal COFF_ObjSection* coff_obj_writer_push_section(COFF_ObjWriter *obj_writer, String8 name, COFF_SectionFlags flags, String8 data);
internal COFF_ObjSymbol*  coff_obj_writer_push_symbol(COFF_ObjWriter *obj_writer, String8 name, U32 value, COFF_SymbolLocation loc, COFF_SymbolType type, COFF_SymStorageClass storage_class);
internal COFF_ObjReloc*   coff_obj_writer_section_push_reloc(COFF_ObjWriter *obj_writer, COFF_ObjSection *sect, U32 apply_off, COFF_ObjSymbol *symbol, COFF_RelocType reloc_type);

#endif // COFF_OBJ_WRITER_H

