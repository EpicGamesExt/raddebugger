// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// Def -> COFF

#define T_COFF_DefSetMachine(v)   &(COFF_MachineType){ COFF_MachineType_##v }
#define T_COFF_DefSetTimeStamp(v) &(COFF_TimeStamp){ v }
#define T_COFF_DefGetMachine(v)   (v ? *v : COFF_MachineType_X64)
#define T_COFF_DefGetTimeStamp(v) (v ? *v : 0)

typedef enum
{
  T_COFF_DefSymbol_Null,
  T_COFF_DefSymbol_Extern,
  T_COFF_DefSymbol_ExternFunc,
  T_COFF_DefSymbol_Static,
  T_COFF_DefSymbol_Secdef,
  T_COFF_DefSymbol_Associative,
  T_COFF_DefSymbol_Weak,
  T_COFF_DefSymbol_Abs,
  T_COFF_DefSymbol_Undef,
  T_COFF_DefSymbol_UndefFunc,
  T_COFF_DefSymbol_UndefSec,
  T_COFF_DefSymbol_Sect,
  T_COFF_DefSymbol_Common,
} T_COFF_DefSymbolType;

typedef struct
{
  T_COFF_DefSymbolType  type;
  char                 *name;
  char                 *section;
  char                 *head;
  char                 *associate;
  char                 *tag;
  U64                   value;
  U64                   size;
  COFF_ComdatSelectType selection;
  COFF_WeakExtType      characteristics;
  COFF_SymStorageClass  storage_class;
} T_COFF_DefSymbol;

#define T_COFF_DefSymbol_Abs(name_, value_, storage_class_, ...)  { .type = T_COFF_DefSymbol_Abs,        .name = name_, .value = value_, .storage_class = storage_class_,##__VA_ARGS__ }
#define T_COFF_DefSymbol_AbsExtern(name_, value_, ...)            T_COFF_DefSymbol_Abs(name_, value_, COFF_SymStorageClass_External, ## __VA_ARGS__)
#define T_COFF_DefSymbol_AbsStatic(name_, value_, ...)            T_COFF_DefSymbol_Abs(name_, value_, COFF_SymStorageClass_Static  , ## __VA_ARGS__)
#define T_COFF_DefSymbol_Associative(head_, associate_, ...)      { .type = T_COFF_DefSymbol_Associative, .head = head_, .associate = associate_                        , ## __VA_ARGS__ }
#define T_COFF_DefSymbol_Common(name_, size_, ...)                { .type = T_COFF_DefSymbol_Common,     .name = name_, .size = size_                                   , ## __VA_ARGS__ }
#define T_COFF_DefSymbol_Extern(name_, section_, value_, ...)     { .type = T_COFF_DefSymbol_Extern,     .name = name_, .section = section_, .value = value_            , ## __VA_ARGS__ }
#define T_COFF_DefSymbol_ExternFunc(name_, section_, value_, ...) { .type = T_COFF_DefSymbol_ExternFunc, .name = name_, .section = section_, .value = value_            , ## __VA_ARGS__ }
#define T_COFF_DefSymbol_Secdef(section_, selection_, ...)        { .type = T_COFF_DefSymbol_Secdef, .section = section_, .selection = selection_                       , ## __VA_ARGS__ }
#define T_COFF_DefSymbol_Sect(name_, section_, ...)               { .type = T_COFF_DefSymbol_Sect,       .name = name_, .section = section_                             , ## __VA_ARGS__ }
#define T_COFF_DefSymbol_Static(name_, section_, value_, ...)     { .type = T_COFF_DefSymbol_Static,     .name = name_, .section = section_, .value = value_            , ## __VA_ARGS__ }
#define T_COFF_DefSymbol_Undef(name_, ...)                        { .type = T_COFF_DefSymbol_Undef,      .name = name_                                                  , ## __VA_ARGS__ }
#define T_COFF_DefSymbol_UndefFunc(name_, ...)                    { .type = T_COFF_DefSymbol_UndefFunc,  .name = name_                                                  , ## __VA_ARGS__ }
#define T_COFF_DefSymbol_UndefSec(name_, value_, ...)             { .type = T_COFF_DefSymbol_UndefSec,   .name = name_, .value = value_                                 , ## __VA_ARGS__ }
#define T_COFF_DefSymbol_Weak(name_, characteristics_, tag_, ...) { .type = T_COFF_DefSymbol_Weak,       .name = name_, .characteristics = characteristics_, .tag = tag_, ## __VA_ARGS__ }

#define T_COFF_DefReloc_SetType(v)  &(COFF_RelocType){ COFF_Reloc_##v }
#define T_COFF_DefReloc(type_, apply_off_, symbol_) { .type = T_COFF_DefReloc_SetType(type_), .apply_off = apply_off_, .symbol = symbol_ }
typedef struct
{
  COFF_RelocType *type;
  U64             apply_off;
  char           *symbol;
} T_COFF_DefReloc;

typedef struct
{
  char              *id;
  char              *name;
  String8            data;
  char              *flags;
  COFF_SectionFlags  raw_flags;
  T_COFF_DefReloc   *relocs;
} T_COFF_DefSection;

typedef struct
{
  COFF_MachineType  *machine;
  COFF_TimeStamp    *time_stamp;
  T_COFF_DefSection *sections;
  T_COFF_DefSymbol  *symbols;
  char             **directives;
  String8            path;
} T_COFF_DefObj;

typedef struct
{
  char              *dll;
  char              *name;
  COFF_ImportByType  import_by;
  COFF_ImportType    type;
  U64                hit_or_ordinal;
  COFF_TimeStamp    *time_stamp;
  COFF_MachineType  *machine;
} T_COFF_DefImport;

typedef struct
{
  char             *name;
  COFF_MachineType *machine;
  COFF_TimeStamp   *time_stamp;
} T_COFF_DefDllImport;

typedef enum
{
  T_COFF_DefLibMember_Null,
  T_COFF_DefLibMember_Obj,
  T_COFF_DefLibMember_Import,
  T_COFF_DefLibMember_DllImportStatic,
} T_COFF_DefLibMemberType;

typedef struct
{
  T_COFF_DefLibMemberType type;
  union {
    T_COFF_DefObj       obj;
    T_COFF_DefImport    import;
    T_COFF_DefDllImport dll_import;
  };
} T_COFF_DefLibMember;

typedef struct
{
  U16                  mode;
  B32                  emit_second_member;
  T_COFF_DefLibMember *members;
} T_COFF_DefLib;

internal COFF_ObjSection *
t_coff_from_def_require_section(HashTable *section_ht, String8 id)
{
  COFF_ObjSection *result = hash_table_search_string_raw(section_ht, id);
  AssertAlways(result != 0);
  return result;
}

internal COFF_ObjSymbol *
t_coff_from_def_require_symbol(HashTable *symbol_ht, String8 id)
{
  COFF_ObjSymbol *result = hash_table_search_string_raw(symbol_ht, id);
  AssertAlways(result != 0);
  return result;
}

internal COFF_SectionFlags
t_coff_section_flags_from_cstr(char *v)
{
  COFF_SectionFlags f = 0;
  for (char *p = v; p && *p; ++p) {
    switch (*p) {
    case 'r': f |= COFF_SectionFlag_MemRead; break;
    case 'w': f |= COFF_SectionFlag_MemWrite; break;
    case 'x': f |= COFF_SectionFlag_MemExecute; break;
    case ':': {
      char *type_begin = p + 1;
      char *type_end = type_begin;
      for (; *type_end && *type_end != '@'; ++type_end) {}
      String8 type = str8((U8 *)type_begin, (U64)(type_end - type_begin));
      if (str8_match(type, str8_lit("bss"), StringMatchFlag_CaseInsensitive)) {
        f |= COFF_SectionFlag_CntUninitializedData;
      } else if (str8_match(type, str8_lit("data"), StringMatchFlag_CaseInsensitive)) {
        f |= COFF_SectionFlag_CntInitializedData;
      } else if (str8_match(type, str8_lit("code"), StringMatchFlag_CaseInsensitive)) {
        f |= COFF_SectionFlag_CntCode;
      }
      p = type_end - 1;
    } break;
    case '@': {
      char *align_begin = p + 1;
      char *align_end = align_begin;
      for (; char_is_digit(*align_end, 10); ++align_end) {}
      AssertAlways(align_end > align_begin);
      U64 align = u64_from_str8(str8((U8 *)align_begin, (U64)(align_end - align_begin)), 10);
      COFF_SectionFlags align_flags = coff_section_flag_from_align_size(align);
      AssertAlways(align_flags != 0);
      f |= align_flags;
      p = align_end - 1;
    } break;
    default: AssertAlways(0 && "unknown flag"); break;
    }
  }
  return f;
}

internal String8
t_coff_from_def_obj(Arena *arena, T_COFF_DefObj obj)
{
  Temp scratch = scratch_begin(&arena, 1);
  HashTable *section_ht = hash_table_init(scratch.arena, 1000);
  HashTable *symbol_ht  = hash_table_init(scratch.arena, 1000);
  COFF_ObjWriter *writer = coff_obj_writer_alloc(T_COFF_DefGetTimeStamp(obj.time_stamp), T_COFF_DefGetMachine(obj.machine));

  // push sections
  for (T_COFF_DefSection *section = obj.sections; section && section->id != 0; ++section) {
    String8 id   = str8_cstring(section->id);
    String8 name = str8_cstring(section->name);
    if (hash_table_search_string_raw(section_ht, id) == 0) {
      COFF_SectionFlags flags = section->raw_flags;
      flags |= t_coff_section_flags_from_cstr(section->flags);
      COFF_ObjSection *sect = coff_obj_writer_push_section(writer, name, flags, section->data);
      hash_table_push_string_raw(scratch.arena, section_ht, id, sect);
    } else {
      AssertAlways(!"error: identifier is already reserved");
    }
  }

  // push directives
  for (char **dir = obj.directives; dir && *dir; ++dir) {
    coff_obj_writer_push_directive(writer, str8_cstring(*dir));
  }

  // push symbols
  for (T_COFF_DefSymbol *symbol = obj.symbols; symbol && symbol->type != 0; ++symbol) {
    String8 name      = str8_cstring(symbol->name);
    String8 section   = str8_cstring(symbol->section);
    String8 head      = str8_cstring(symbol->head);
    String8 associate = str8_cstring(symbol->associate);
    String8 tag       = str8_cstring(symbol->tag);

    COFF_ObjSymbol *ptr = 0;
    switch (symbol->type) {
    case T_COFF_DefSymbol_ExternFunc:  { ptr = coff_obj_writer_push_symbol_extern_func(writer, name, safe_cast_u32(symbol->value), t_coff_from_def_require_section(section_ht, section));                 } break;
    case T_COFF_DefSymbol_Extern:      { ptr = coff_obj_writer_push_symbol_extern     (writer, name, safe_cast_u32(symbol->value), t_coff_from_def_require_section(section_ht, section));                  } break;
    case T_COFF_DefSymbol_Static:      { ptr = coff_obj_writer_push_symbol_static     (writer, name, safe_cast_u32(symbol->value), t_coff_from_def_require_section(section_ht, section));                  } break;
    case T_COFF_DefSymbol_Secdef:      { ptr = coff_obj_writer_push_symbol_secdef     (writer, t_coff_from_def_require_section(section_ht, section), symbol->selection);                                   } break;
    case T_COFF_DefSymbol_Associative: { ptr = coff_obj_writer_push_symbol_associative(writer, t_coff_from_def_require_section(section_ht, head), t_coff_from_def_require_section(section_ht, associate)); } break;
    case T_COFF_DefSymbol_Weak:        { ptr = coff_obj_writer_push_symbol_weak       (writer, name, symbol->characteristics, t_coff_from_def_require_symbol(symbol_ht, tag));                             } break;
    case T_COFF_DefSymbol_Abs:         { ptr = coff_obj_writer_push_symbol_abs        (writer, name, safe_cast_u32(symbol->value), symbol->storage_class);                                                 } break;
    case T_COFF_DefSymbol_Undef:       { ptr = coff_obj_writer_push_symbol_undef      (writer, name);                                                                                                      } break;
    case T_COFF_DefSymbol_UndefFunc:   { ptr = coff_obj_writer_push_symbol_undef_func (writer, name);                                                                                                      } break;
    case T_COFF_DefSymbol_UndefSec:    { ptr = coff_obj_writer_push_symbol_undef_sect (writer, name, safe_cast_u32(symbol->value));                                                                        } break;
    case T_COFF_DefSymbol_Sect:        { ptr = coff_obj_writer_push_symbol_sect       (writer, name, t_coff_from_def_require_section(section_ht, section));                                                } break;
    case T_COFF_DefSymbol_Common:      { ptr = coff_obj_writer_push_symbol_common     (writer, name, safe_cast_u32(symbol->size));                                                                         } break;
    default: { InvalidPath; } break;
    }

    if (name.size != 0) {
      if (hash_table_search_string_raw(symbol_ht, name) == 0) {
        hash_table_push_string_raw(scratch.arena, symbol_ht, name, ptr);
      } else {
        AssertAlways(!"error: identifier is already reserved");
      }
    }
  }

  // push relocs
  for (T_COFF_DefSection *section = obj.sections; section && section->id != 0; ++section) {
    COFF_ObjSection *sect = t_coff_from_def_require_section(section_ht, str8_cstring(section->id));
    for (T_COFF_DefReloc *reloc = section->relocs; reloc && reloc->type != 0; ++reloc) {
      COFF_ObjSymbol *symbol = t_coff_from_def_require_symbol(symbol_ht, str8_cstring(reloc->symbol));
      coff_obj_writer_section_push_reloc(writer, sect, safe_cast_u32(reloc->apply_off), symbol, *reloc->type);
    }
  }

  String8 result = coff_obj_writer_serialize(arena, writer);
  coff_obj_writer_release(&writer);
  scratch_end(scratch);
  return result;
}

internal String8
t_coff_from_def_lib(Arena *arena, T_COFF_DefLib lib)
{
  COFF_LibWriter *writer = coff_lib_writer_alloc();
  COFF_TimeStamp time_stamp = 0;

  for (T_COFF_DefLibMember *member = lib.members; member && member->type != 0; ++member) {
    switch (member->type) {
    case T_COFF_DefLibMember_Obj:
    {
      T_COFF_DefObj obj  = member->obj;
      String8       path = obj.path.size == 0 ? push_str8f(arena, "member_%llu.obj", (U64)(member - lib.members)) : obj.path;
      String8       data = t_coff_from_def_obj(arena, obj);
      coff_lib_writer_push_obj(writer, path, data);
    } break;
    case T_COFF_DefLibMember_Import:
    {
      T_COFF_DefImport import = member->import;
      coff_lib_writer_push_import(writer,
                                  T_COFF_DefGetMachine(import.machine),
                                  T_COFF_DefGetTimeStamp(import.time_stamp),
                                  str8_cstring(import.dll),
                                  import.import_by,
                                  str8_cstring(import.name),
                                  safe_cast_u16(import.hit_or_ordinal),
                                  import.type);
    } break;
    case T_COFF_DefLibMember_DllImportStatic: {
      String8 dll_name          = str8_chop_last_dot(str8_cstring(member->dll_import.name));
      String8 null_import_debug = lnk_make_linker_debug_symbols(writer->arena, T_COFF_DefGetMachine(member->dll_import.machine));
      String8 import_entry_obj = pe_make_import_entry_obj(writer->arena,
                                                           dll_name,
                                                           T_COFF_DefGetTimeStamp(member->dll_import.time_stamp),
                                                           T_COFF_DefGetMachine(member->dll_import.machine),
                                                           null_import_debug);
      String8 null_import_obj = pe_make_null_import_descriptor_obj(writer->arena,
                                                                    T_COFF_DefGetTimeStamp(member->dll_import.time_stamp),
                                                                    T_COFF_DefGetMachine(member->dll_import.machine),
                                                                    null_import_debug);
      String8 null_thunk_obj = pe_make_null_thunk_data_obj(writer->arena,
                                                           dll_name,
                                                           T_COFF_DefGetTimeStamp(member->dll_import.time_stamp),
                                                           T_COFF_DefGetMachine(member->dll_import.machine),
                                                           null_import_debug);
      coff_lib_writer_push_obj(writer, dll_name, import_entry_obj);
      coff_lib_writer_push_obj(writer, dll_name, null_import_obj);
      coff_lib_writer_push_obj(writer, dll_name, null_thunk_obj);
    } break;
    case T_COFF_DefLibMember_Null: break;
    default: InvalidPath; break;
    }
  }

  String8 result = coff_lib_writer_serialize(arena, writer, time_stamp, lib.mode, lib.emit_second_member);
  coff_lib_writer_release(&writer);
  return result;
}

internal B32
t_write_def_obj(char *path, T_COFF_DefObj obj)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_ok = t_write_file(str8_cstring(path), t_coff_from_def_obj(scratch.arena, obj));
  scratch_end(scratch);
  return is_ok;
}

////////////////////////////////

internal String8
t_make_entry_obj(Arena *arena)
{
  return t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\xc3"), .flags = "rx:code@1" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]) {
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      {0}
    }
  });
}

internal B32
t_write_entry_obj(void)
{
  Temp scratch = scratch_begin(0,0);
  String8 obj   = t_make_entry_obj(scratch.arena);
  B32     is_ok = t_write_file(str8_lit("entry.obj"), obj);
  scratch_end(scratch);
  return is_ok;
}

internal String8
data_from_pdb(Arena *arena, PDB_Context *pdb)
{
  TP_Context *tp       = tp_alloc(arena, 1, 1, str8_lit("foo"));
  TP_Arena   *tp_arena = tp_arena_alloc(tp);
  pdb_build(tp, tp_arena, pdb, (CV_StringHashTable){0}, 1, 0, 0);

  AssertAlways(msf_build(pdb->msf) == MSF_Error_OK);
  String8List raw_msf_list = msf_get_page_data_nodes(arena, pdb->msf);
  AssertAlways(t_write_file_list(str8_lit("test.pdb"), raw_msf_list));

  String8 data = str8_list_join(arena, &raw_msf_list, 0);

  tp_arena_release(&tp_arena);
  tp_release(tp);

  return data;
}
