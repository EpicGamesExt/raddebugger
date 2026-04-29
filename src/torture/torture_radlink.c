// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal T_Linker
t_id_linker(void)
{
  String8 name = str8_chop_last_dot(str8_skip_last_slash(g_linker));
  if (str8_match(name, str8_lit("radlink"),  StringMatchFlag_CaseInsensitive)) { return T_Linker_RAD;  }
  if (str8_match(name, str8_lit("link"),     StringMatchFlag_CaseInsensitive)) { return T_Linker_MSVC; }
  if (str8_match(name, str8_lit("lld-link"), StringMatchFlag_CaseInsensitive)) { return T_Linker_LLVM; }
  return T_Linker_Null;
}

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

typedef enum
{
  T_COFF_DefRootType_Null,
  T_COFF_DefRootType_Obj,
  T_COFF_DefRootType_Lib
} T_COFF_DefRootType;

typedef struct
{
  T_COFF_DefRootType type;
  union {
    T_COFF_DefObj obj;
    T_COFF_DefLib lib;
  };
} T_COFF_DefRoot;

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

internal String8
t_coff_from_def_root(Arena *arena, T_COFF_DefRoot root)
{
  switch (root.type) {
  case T_COFF_DefRootType_Obj: return t_coff_from_def_obj(arena, root.obj);
  case T_COFF_DefRootType_Lib: return t_coff_from_def_lib(arena, root.lib);
  case T_COFF_DefRootType_Null: break;
  default: { NotImplemented; } break;
  }
  return str8_zero();
}

internal B32
t_write_def_obj(char *path, T_COFF_DefObj obj)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_ok = t_write_file(str8_cstring(path), t_coff_from_def_obj(scratch.arena, obj));
  scratch_end(scratch);
  return is_ok;
}

internal B32
t_write_def_lib(char *path, T_COFF_DefLib lib)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_ok = t_write_file(str8_cstring(path), t_coff_from_def_lib(scratch.arena, lib));
  scratch_end(scratch);
  return is_ok;
}

////////////////////////////////

internal String8
t_make_sec_defn_obj(Arena *arena, String8 payload)
{
  return t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "mysect", ".mysect", payload, .flags = "r:data@1" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("mysect", COFF_ComdatSelect_Null),
      {0}
    }
  });
}

internal String8
t_make_obj_with_directive(Arena *arena, String8 directive)
{
  String8 directive_cstr = push_cstr(arena, directive);
  return t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .directives = (char *[]){ (char *)directive_cstr.str, 0 }
  });
}

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

////////////////////////////////

#define T_Group "Linker" 

TEST(machine_compat_check)
{
  // unknown.obj
  T_Ok(t_write_def_obj("unknown.obj", (T_COFF_DefObj){
    .machine = &(COFF_MachineType){ COFF_MachineType_Unknown },
    .sections = (T_COFF_DefSection[]){
      { "data", ".data", str8_lit("unknown"), .flags = "rw:data" },
      {0}
    }
  }));

  // x64.obj
  T_Ok(t_write_def_obj("x64.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "data", ".data", str8_lit("x64"), .flags = "rw:data" },
      {0}
    }
  }));

  // entry.obj
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\xc3"), .flags = "rx:code" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      {0}
    }
  }));

  // arm64.obj
  T_Ok(t_write_def_obj("arm64.obj", (T_COFF_DefObj){
    .machine = &(COFF_MachineType){ COFF_MachineType_Arm64 },
    .sections = (T_COFF_DefSection[]){
      { "data", ".data", str8_lit("arm64"), .flags = "rw:data" },
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe entry.obj unknown.obj x64.obj");
  T_Ok(g_last_exit_code == 0);

  // test objs with conflicting machines
  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe entry.obj unknown.obj x64.obj arm64.obj");
  T_Ok(g_last_exit_code != 0);

  // check /MACHINE switch
  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe /machine:amd64 arm64.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
}

TEST(simple_link_test)
{
  U8 text_payload[] = { 0xC3 };

  String8 main_obj;
  {
    main_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "text", ".text", str8_array_fixed(text_payload), .flags = "rx:code" },
        { "data", ".data", str8_lit("qwe"), .flags = "rw:data" },
        { "zero", ".zero", str8(0, 5), .flags = "rw:bss" },
        {0}
      },
      .symbols = (T_COFF_DefSymbol[]){
        T_COFF_DefSymbol_Extern("my_entry", "text", 0),
        {0}
      }
    });
  }

  String8 main_obj_name = str8_lit("main.obj");
  T_Ok(t_write_file(main_obj_name, main_obj));

  int file_align = 512;
  int virt_align = 4096;
  String8 out_name = str8_lit("a.exe");
  t_invoke_linkerf("/entry:my_entry /subsystem:console /fixed /filealign:%d /align:%d /out:%S %S", file_align, virt_align, out_name, main_obj_name);
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, out_name);
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  T_Ok(!pe.is_pe32);
  T_Ok(pe.section_count == 3);
  T_Ok(pe.arch == Arch_x64);
  T_Ok(pe.subsystem == PE_WindowsSubsystem_WINDOWS_CUI);
  T_Ok(pe.virt_section_align == virt_align);
  T_Ok(pe.file_section_align == file_align);
  T_Ok(pe.symbol_count == 0);
  T_Ok(pe.data_dir_count == PE_DataDirectoryIndex_COUNT);

  // check section alignment
  for EachIndex(sect_idx, pe.section_count) {
    COFF_SectionHeader *sect_header = &section_table[sect_idx];
    T_Ok(AlignPadPow2(sect_header->fsize, file_align) == 0);
    T_Ok(AlignPadPow2(sect_header->voff, virt_align) == 0);
  }

  COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
  T_Ok(text_section != 0);
  T_Ok(text_section->foff == file_align);
  T_Ok(pe.entry_point == text_section->voff);

  COFF_SectionHeader *data_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
  T_Ok(data_section != 0);

  COFF_SectionHeader *zero_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".zero"));
  T_Ok(zero_section != 0);

  String8 text_data = str8_substr(exe, rng_1u64(text_section->foff, text_section->foff + text_section->vsize));
  T_Ok(str8_match(text_data, str8_array_fixed(text_payload), 0));

  PE_OptionalHeader32Plus *opt = str8_deserial_get_raw_ptr(exe, pe.optional_header_off, sizeof(*opt));
  T_Ok(opt->sizeof_code == text_section->fsize);
  T_Ok(opt->sizeof_inited_data == (text_section->fsize + data_section->fsize));
  T_Ok(opt->sizeof_uninited_data == 0x200);
  T_Ok(opt->code_base == 0x1000);
  T_Ok(opt->image_base == 0x140000000);
  T_Ok(opt->major_os_ver == 6);
  T_Ok(opt->minor_os_ver == 0);
  T_Ok(opt->major_img_ver == 0);
  T_Ok(opt->minor_img_ver == 0);
  T_Ok(opt->major_subsystem_ver == 6);
  T_Ok(opt->minor_subsystem_ver == 0);
  T_Ok(opt->win32_version_value == 0);
  T_Ok(opt->sizeof_image == 0x4000);
  T_Ok(opt->sizeof_headers == 0x200);
  T_Ok(opt->dll_characteristics == 0x8120);
  T_Ok(opt->loader_flags == 0);
}


TEST(out_of_bounds_section_number)
{
  // bad.obj
  {
    String8 obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "foo", ".foo", str8_lit("foo"), .flags = "rw:data" },
        {0}
      },
      .symbols = (T_COFF_DefSymbol[]){
        T_COFF_DefSymbol_Extern("foo", "foo", 0),
        {0}
      }
    });
    {
      COFF_FileHeaderInfo header = coff_file_header_info_from_data(obj);
      String8 string_table = str8_substr(obj, header.string_table_range);
      String8 symbol_table = str8_substr(obj, header.symbol_table_range);
      COFF_ParsedSymbol symbol = coff_parse_symbol(header, string_table, symbol_table, 0);
      COFF_Symbol16 *symbol16 = symbol.raw_symbol;
      symbol16->section_number = 123;
    }
    T_Ok(t_write_file(str8_lit("bad.obj"), obj));
  }

  // entry.obj
  {
    String8 obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        {
          "text", ".text",
          str8_lit_comp("\x48\xC7\xC0\x00\x00\x00\x00\xC3"),
          .flags = "rx:code",
          .relocs = (T_COFF_DefReloc[]){
            T_COFF_DefReloc(X64_Addr32Nb, 0, "foo"),
            {0}
          }
        },
        {0}
      },
      .symbols = (T_COFF_DefSymbol[]){
        T_COFF_DefSymbol_Extern("entry", "text", 0),
        T_COFF_DefSymbol_Undef("foo"),
        {0}
      }
    });
    T_Ok(t_write_file(str8_lit("entry.obj"), obj));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj bad.obj");
  T_Ok(g_last_exit_code != 0);
}


TEST(merge)
{
  T_Ok(t_write_def_obj("test.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "test", ".test", str8_lit("hello, world"), .flags = "rw:data" },
      {0}
    }
  }));

  T_Ok(t_write_entry_obj());

  // circular merge
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.test=.test entry.obj test.obj");
  T_Ok(g_last_exit_code != 0);

  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_CircularMerge);
  }

  // circular merge with extra link
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.test=.data /merge:.data=.test entry.obj test.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_CircularMerge);
  }

  // merge with non-defined section
  {
    g_last_exit_code;

    t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.test=.qwe entry.obj test.obj");
    T_Ok(g_last_exit_code == 0);

    // make sure linker created .qwe and merged .test into it
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".qwe"));
    T_Ok(sect != 0);
    T_Ok(sect->flags == PE_DATA_SECTION_FLAGS);
    String8 qwe = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(qwe, str8_lit("hello, world"),0));
  }

  // illegal merge with .reloc
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.test=.reloc entry.obj test.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_IllegalSectionMerge);
  }

  // illegal merge with .rsrc
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.test=.rsrc entry.obj test.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_IllegalSectionMerge);
  }

  // merge non-defined section with defined section
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.qwe=.test entry.obj test.obj");
  T_Ok(g_last_exit_code == 0);

  // merge .test -> .qwe -> .data
  {
    t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /merge:.test=.qwe /merge:.qwe=.data entry.obj test.obj");
    T_Ok(g_last_exit_code == 0);

    // make sure linker merged .test into .data
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".data"));
    T_Ok(sect != 0);
    T_Ok(sect->flags == PE_DATA_SECTION_FLAGS);
    String8 data = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("hello, world"),0));
  }
}

TEST(link_undef)
{
  T_Ok(t_write_def_obj("undef.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("undef"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp("\x48\xC7\xC0\x00\x00\x00\x00\xC3"),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "undef"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("undef"),
      {0}
    }
  }));

  // try linking unresolved symbol and see if linker picks up on that
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj undef.obj");
  T_Ok(g_last_exit_code == LNK_Error_UnresolvedSymbol);
}

TEST(link_unref_undef)
{
  T_Ok(t_write_def_obj("undef.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("undef"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\xc3"), .flags = "rx:code" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      {0}
    }
  }));

  // try linking unreferenced unresolved symbol, this must link
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj undef.obj");
  T_Ok(g_last_exit_code != 0);
}

TEST(weak_lib_vs_weak_lib)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchLibrary, "q"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchLibrary, "e"),
      {0}
    }
  }));

  // linker must pick weak symbol from a.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x111;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_lib_vs_weak_nolib)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_NoLibrary, "q"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchLibrary, "e"),
      {0}
    }
  }));

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_lib_vs_weak_alias)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchAlias, "q"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchLibrary, "e"),
      {0}
    }
  }));

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol);
}

TEST(weak_lib_vs_weak_antidep)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_AntiDependency, "q"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchLibrary, "e"),
      {0}
    }
  }));

  // linker must pick weak symbol from a.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_alias_vs_weak_alias)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("qwe", 0x111),
      T_COFF_DefSymbol_Weak("sym", COFF_WeakExt_SearchAlias, "qwe"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("b.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("ewq", 0x222),
      T_COFF_DefSymbol_Weak("sym", COFF_WeakExt_SearchAlias, "ewq"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "sym"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("sym"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol);
}

TEST(weak_alias_vs_weak_lib)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_AntiDependency, "q"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00"
          "\xC3"
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchAlias, "e"),
      {0}
    }
  }));

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_alias_vs_weak_nolib)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_NoLibrary, "q"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00"
          "\xC3"
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchAlias, "e"),
      {0}
    }
  }));

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_alias_vs_weak_antidep)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_AntiDependency, "q"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00"
          "\xC3"
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchAlias, "e"),
      {0}
    }
  }));

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_nolib_vs_weak_nolib)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_NoLibrary, "q"),
      {0}
    }
  }));
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
                                       "\xC3"), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_NoLibrary, "e"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_nolib_vs_weak_lib)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchLibrary, "q"),
      {0}
    }
  }));
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
                                       "\xC3"), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_NoLibrary, "e"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_nolib_vs_weak_alias)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchAlias, "q"),
      {0}
    }
  }));
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
                                       "\xC3"), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_NoLibrary, "e"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol);
}

TEST(weak_nolib_vs_weak_antidep)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_AntiDependency, "q"),
      {0}
    }
  }));
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
                                       "\xC3"), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_NoLibrary, "e"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_antidep_vs_weak_antidep)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_AntiDependency, "q"),
      {0}
    }
  }));
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
                                       "\xC3"), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_AntiDependency, "e"),
      {0}
    }
  }));

  // linker must pick weak symbol from a.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x111;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }

  // linker must pick weak symbol from entry.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_antidep_vs_weak_nolib)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_NoLibrary, "q"),
      {0}
    }
  }));
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
                                       "\xC3"), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_AntiDependency, "e"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_antidep_vs_weak_lib)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchLibrary, "q"),
      {0}
    }
  }));
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
                                       "\xC3"), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_AntiDependency, "e"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x222;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_antidep_vs_weak_alias)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("q", 0x111),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchAlias, "q"),
      {0}
    }
  }));
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
                                          "\xC3"), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 3, "w"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_AbsExtern("e", 0x222),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_AntiDependency, "e"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
    T_Ok(text_sect != 0);
    String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));
    String8 imm       = str8_substr(text_data, rng_1u64(3, 7));
    U32 expected = 0x111;
    T_Ok(str8_match(imm, str8_struct(&expected), 0));
  }
}

TEST(weak_vs_common)
{
  T_Ok(t_write_def_obj("weak.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){ { "a", ".a", str8_lit("a"), .flags = "rw:data" }, {0} },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Static("_a", "a", 0),
      T_COFF_DefSymbol_Weak("w", COFF_WeakExt_SearchLibrary, "_a"),
      {0}
    }
  }));
  T_Ok(t_write_def_obj("common.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){ T_COFF_DefSymbol_Common("w", 2),  {0} }
  }));
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3"
        ), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){ T_COFF_DefReloc(X64_Addr32Nb, 0, "w"), {0} }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("w"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe common.obj weak.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe weak.obj common.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *bss = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".bss"));
  T_Ok(bss);
  T_Ok(bss->fsize == 0);
  T_Ok(bss->vsize == 2);
}

TEST(abs_vs_weak)
{
  U32 abs_value   = 0x123;
  U8  text_code[] = { 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC3 };

  T_Ok(t_write_def_obj("abs.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("foo", abs_value),
      {0},
    }
  }));

  T_Ok(t_write_def_obj("text.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "mydata", ".mydata", str8_lit("mydata"), .flags = "rx:code@1" },
      {
        "text", ".text", str8_array_fixed(text_code), .flags = "rx:code@1",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr64, 2, "foo"),
          {0},
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("mydata", "mydata", 0),
      T_COFF_DefSymbol_Weak("foo", COFF_WeakExt_NoLibrary, "mydata"),
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe abs.obj text.obj");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe text.obj abs.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
  T_Ok(text_section != 0);

  String8 text_data = str8_substr(exe, rng_1u64(text_section->foff, text_section->foff + text_section->fsize));
  String8 inst      = str8_prefix(text_data, 2);
  T_Ok(str8_match(inst, str8_array(text_code, 2), 0));

  String8 imm          = str8_prefix(str8_skip(text_data, 2), 8);
  U64     expected_imm = abs_value;
  T_Ok(str8_match(imm, str8_struct(&expected_imm), 0));
}

TEST(abs_vs_regular)
{
  String8 shared_symbol_name = str8_lit("foo");

  U8 regular_payload[] = { 0xC0, 0xFF, 0xEE };
  String8 regular_obj_name = str8_lit("regular.obj");

  T_Ok(t_write_def_obj("regular.obj", (T_COFF_DefObj){
    .machine  = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){ { "data", ".data", str8_array_fixed(regular_payload), .flags = "rw:data" }, {0}, },
    .symbols  = (T_COFF_DefSymbol[]){ T_COFF_DefSymbol_Extern("foo", "data", 0), {0}, }
  }));

  String8 abs_obj_name = str8_lit("abs.obj");
  T_Ok(t_write_def_obj("abs.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){ T_COFF_DefSymbol_AbsExtern("foo", 0x1234), {0}, }
  }));

  U8 entry_text[] = { 
    0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, // mov rax, $imm
    0xC3 // ret
  };
  String8 entry_obj_name = str8_lit("entry.obj");
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text", str8_array_fixed(entry_text), .flags = "rx:code@1",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 3, "foo"),
          {0},
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      T_COFF_DefSymbol_Undef("foo"),
      {0},
    }
  }));

  // TODO: validate that linker issues multiply defined symbol error
  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe abs.obj regular.obj entry.obj");
  // linker should complain about multiply defined symbol
  T_Ok(g_last_exit_code != 0);

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe regular.obj abs.obj entry.obj");
  // linker should complain even in case regular is before abs
  T_Ok(g_last_exit_code != 0);
}

TEST(abs_vs_common)
{
  String8 shared_symbol_name = str8_lit("foo");

  String8 common_obj_name = str8_lit("common.obj");

  T_Ok(t_write_def_obj("common.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){ T_COFF_DefSymbol_Common("foo", 321), {0}, }
  }));

  String8 abs_obj_name = str8_lit("abs.obj");
  T_Ok(t_write_def_obj("abs.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){ T_COFF_DefSymbol_AbsExtern("foo", 0x1234), {0}, }
  }));

  U8 entry_text[] = { 0xC3 };
  String8 entry_obj_name = str8_lit("entry.obj");
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){ { "text", ".text", str8_array_fixed(entry_text), .flags = "rx:code@1" }, {0}, },
    .symbols = (T_COFF_DefSymbol[]){ T_COFF_DefSymbol_Extern("my_entry", "text", 0), {0}, }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe abs.obj common.obj entry.obj");
  if (g_last_exit_code == 0) {
    // TODO: validate that linker issues multiply defined symbol error
    t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe common.obj abs.obj entry.obj");
    if (t_id_linker() == T_Linker_RAD) {
      T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol);
    } else {
      T_Ok(g_last_exit_code != 0);
    }
  }
}

TEST(abs_vs_abs)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){ T_COFF_DefSymbol_AbsExtern("foo", 'a'), {0}, },
  }));
  T_Ok(t_write_def_obj("b.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){ T_COFF_DefSymbol_AbsExtern("foo", 'b'), {0}, },
  }));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol);
}

TEST(undef_weak_lib)
{
  T_Ok(t_write_def_obj("weak.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsExtern("b", 0xc3000000),
      T_COFF_DefSymbol_Weak("a", COFF_WeakExt_SearchLibrary, "b"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text", str8_lit_comp("\x00\x00\x00\x00"), .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){ T_COFF_DefReloc(X64_Addr32Nb, 0, "a"), {0} }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("a"),
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      {0},
    }
  }));

  // undefined symbol must always replace weak symbol with search library
  t_invoke_linkerf("/subsystem:console /out:a.exe /entry:entry entry.obj weak.obj");
  T_Ok(g_last_exit_code == LNK_Error_UnresolvedSymbol);
}

TEST(undef_weak_search_alias)
{
  Temp scratch = scratch_begin(0,0);

  T_Ok(t_write_def_obj("weak.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){ { "data", ".data", str8_lit_comp("\xde\xad\xbe\xef"), .flags = "rw:data" }, {0} },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("ptr"),
      T_COFF_DefSymbol_Weak("foo", COFF_WeakExt_SearchAlias, "ptr"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("ptr.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("entry"),
      T_COFF_DefSymbol_Weak("ptr", COFF_WeakExt_SearchAlias, "entry"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("undef.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "data", ".data", str8_lit_comp("\x00\x00\x00\x00"), .flags = "rw:data",
        .relocs = (T_COFF_DefReloc[]){ T_COFF_DefReloc(X64_Addr32Nb, 0, "foo"), {0} }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){ T_COFF_DefSymbol_Undef("foo"), {0} }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){ { "text", ".text", str8_lit_comp("\xC3"), .flags = "rx:code@1" }, {0} },
    .symbols = (T_COFF_DefSymbol[]){ T_COFF_DefSymbol_Extern("entry", "text", 0), {0} }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe weak.obj entry.obj ptr.obj undef.obj");
  T_Ok(g_last_exit_code == 0);
}

TEST(weak_cycle)
{
  String8 ab_obj_name = str8_lit("ab.obj");

  T_Ok(t_write_def_obj("ab.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("B"),
      T_COFF_DefSymbol_Weak("A", COFF_WeakExt_SearchAlias, "B"),
      {0},
    }
  }));

  String8 ba_obj_name = str8_lit("ba.obj");
  T_Ok(t_write_def_obj("ba.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("A"),
      T_COFF_DefSymbol_Weak("B", COFF_WeakExt_SearchAlias, "A"),
      {0},
    }
  }));

  String8 entry_obj_name = str8_lit("entry.obj");
  U8 entry_payload[] = { 0xC3 };
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){ { "text", ".text", str8_array_fixed(entry_payload), .flags = "rx:code@1" }, {0} },
    .symbols = (T_COFF_DefSymbol[]){ T_COFF_DefSymbol_Extern("my_entry", "text", 0), {0} }
  }));

  U64 timeout = os_now_microseconds() + 3 * 1000 * 1000; // give a generous 3 seconds
  t_invoke_linker_timeoutf(timeout, "/subsystem:console /entry:my_entry %S %S %S", entry_obj_name, ab_obj_name, ba_obj_name);
}

TEST(weak_tag)
{
  U32     weak_tag_expected_value = 0x12345678;
  String8 weak_tag_obj_name       = str8_lit("weak_tag.obj");

  T_Ok(t_write_def_obj("weak_tag.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "data", ".data", str8_lit_comp("\x00\x00\x00\x00"), .flags = "rw:data",
        .relocs = (T_COFF_DefReloc[]){ T_COFF_DefReloc(X64_Addr32, 0, "strong_second"), {0} }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_AbsStatic("abs", weak_tag_expected_value),
      T_COFF_DefSymbol_Weak("strong_first", COFF_WeakExt_SearchAlias, "abs"),
      T_COFF_DefSymbol_Weak("strong_second", COFF_WeakExt_SearchAlias, "strong_first"),
      {0}
    }
  }));

  String8 entry_name     = str8_lit("my_entry");
  U8      entry_text[]   = { 0xC3 };
  String8 entry_obj_name = str8_lit("entry.obj");
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){ { "text", ".text", str8_array_fixed(entry_text), .flags = "rx:code@1" }, {0} },
    .symbols = (T_COFF_DefSymbol[]){ T_COFF_DefSymbol_Extern((char *)entry_name.str, "text", 0), {0} }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe %S %S", weak_tag_obj_name, entry_obj_name);
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *data_section  = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
  String8             data          = str8_substr(exe, rng_1u64(data_section->foff, data_section->foff + data_section->vsize));
  T_Ok(data_section);
  T_Ok(data_section->vsize == 4);
  T_Ok(str8_match(data, str8_struct(&weak_tag_expected_value), 0));
}

TEST(undef_section)
{
  U8 payload[] = { 1, 2, 3 };
  String8 sec_defn_obj = t_make_sec_defn_obj(arena, str8_array_fixed(payload));

  T_Ok(t_write_def_obj("main.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "data", ".data", str8_lit_comp("\x00\x00\x00\x00"), .flags = "rw:data",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, ".mysect"),
          {0}
        }
      },
      { "text", ".text", str8_lit_comp("\xC3"), .flags = "rx:code@1" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_UndefSec(".mysect", COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead),
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      {0}
    }
  }));
  t_write_file(str8_lit("sec_defn.obj"), sec_defn_obj);

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe main.obj sec_defn.obj");
  if (g_last_exit_code == 0) {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);

    COFF_SectionHeader *data_section   = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
    COFF_SectionHeader *mysect_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".mysect"));
    if (data_section && mysect_section) {
      if (data_section->vsize == 4 && mysect_section->vsize == 3) {
        String8 addr32nb = str8_substr(exe, rng_1u64(data_section->foff, data_section->foff + data_section->vsize));
        String8 expected_voff = str8_struct(&mysect_section->voff);
        T_Ok(str8_match(addr32nb, expected_voff, 0));
      }
    }
  }
}

TEST(sect_symbol)
{
  String8 sect_payload = str8_lit("hello, world");
  T_Ok(t_write_def_obj("sect.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "mysect1", ".mysect$1", sect_payload, .flags = "rw:data@1" },
      {0}
    },
    .directives = (char *[]){ "/merge:.mysect=.data", 0 }
  }));

  T_Ok(t_write_def_obj("main.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "data", ".data", str8_lit_comp("\x00\x00\x00\x00\x00\x00\x00\x00"), .flags = "rw:data",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr64, 0, ".mysect$2222"),
          {0}
        }
      },
      { "text", ".text", str8_lit_comp("\xC3"), .flags = "rx:code@1" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_UndefSec(".mysect$2222", t_coff_section_flags_from_cstr("rw:data")),
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe main.obj sect.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *sect          = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));

  T_Ok(sect != 0);

  String8 sect_data = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));

  String8 addr_data = str8_substr(sect_data, rng_1u64(0, sizeof(U64)));
  T_Ok(addr_data.size == sizeof(U64));

  U64 addr = *(U64 *)addr_data.str;
  T_Ok(addr - (pe.image_base + sect->voff) == 8);

  String8 payload_got = str8_substr(sect_data, rng_1u64(8, sect_data.size));
  T_Ok(str8_match(payload_got, sect_payload, 0));
}

TEST(undef_reloc_section)
{
  U8 payload[] = { 1, 2, 3 };
  String8 sec_defn_obj = t_make_sec_defn_obj(arena, str8_array_fixed(payload));

  T_Ok(t_write_def_obj("main.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\xC3"), .flags = "rx:code" },
      {
        "data", ".data", str8_lit_comp("\x00\x00\x00\x00\x00\x00\x00\x00"), .flags = "rw:data",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr64, 0, ".reloc"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      T_COFF_DefSymbol_UndefSec(".reloc", PE_RELOC_SECTION_FLAGS),
      {0}
    }
  }));
  T_Ok(t_write_file(str8_lit("sec_defn.obj"), sec_defn_obj));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe main.obj sec_defn.obj");
  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_SectRefsDiscardedMemory);
  } else {
    T_Ok(g_last_exit_code != 0);
  }
}

TEST(find_merged_pdata)
{
  U8 foobar_payload[] = {
    0x40, 0x57, 0x48, 0x81, 0xEC, 0x00, 0x02, 0x00, 0x00, 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00,
    0x48, 0x33, 0xC4, 0x48, 0x89, 0x84, 0x24, 0xF0, 0x01, 0x00, 0x00, 0x48, 0x8D, 0x04, 0x24, 0x48,
    0x8B, 0xF8, 0x33, 0xC0, 0xB9, 0xEC, 0x01, 0x00, 0x00, 0xF3, 0xAA, 0xB8, 0x04, 0x00, 0x00, 0x00,
    0x48, 0x6B, 0xC0, 0x02, 0x8B, 0x04, 0x04, 0x48, 0x8B, 0x8C, 0x24, 0xF0, 0x01, 0x00, 0x00, 0x48,
    0x33, 0xCC, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x81, 0xC4, 0x00, 0x02, 0x00, 0x00, 0x5F, 0xC3,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0x48, 0x83, 0xEC, 0x28, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x28, 0xC3      
  };
  U8 xdata_payload[] = {
    0x19, 0x1B, 0x03, 0x00, 0x09, 0x01, 0x40, 0x00, 0x02, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xF0, 0x01, 0x00, 0x00, 0x01, 0x04, 0x01, 0x00, 0x04, 0x42, 0x00, 0x00
  };
  PE_IntelPdata intel_pdata = {0};
  U8 text_payload[]  = { 0xC3 };

  T_Ok(t_write_def_obj("main.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "xdata", ".xdata", str8_array_fixed(xdata_payload), .flags = "r:data@4" },
      {
        "pdata", ".pdata", str8_struct(&intel_pdata), .flags = "r:data@4",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, OffsetOf(PE_IntelPdata, voff_unwind_info), "$unwind$foobar"),
          T_COFF_DefReloc(X64_Addr32Nb, OffsetOf(PE_IntelPdata, voff_first), "foobar"),
          T_COFF_DefReloc(X64_Addr32Nb, OffsetOf(PE_IntelPdata, voff_one_past_last), "foobar"),
          {0}
        }
      },
      { "foobar", ".foobar", str8_array_fixed(foobar_payload), .flags = "rx:code@1" },
      { "text", ".text", str8_array_fixed(text_payload), .flags = "rx:code@1" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Static("foobar", "foobar", 0),
      T_COFF_DefSymbol_Secdef("xdata", COFF_ComdatSelect_Null),
      T_COFF_DefSymbol_Static("$unwind$foobar", "xdata", 0),
      T_COFF_DefSymbol_Secdef("pdata", COFF_ComdatSelect_Null),
      T_COFF_DefSymbol_Static("$pdata$foobar", "pdata", 0),
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe main.obj /merge:.pdata=.rdata");
  T_Ok(g_last_exit_code == 0);

  String8    exe = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo pe  = pe_bin_info_from_data(arena, exe);
  T_Ok(dim_1u64(pe.data_dir_franges[PE_DataDirectoryIndex_EXCEPTIONS]) == 0xC);
}

TEST(section_sort)
{
  COFF_SectionFlags data_flags = COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemRead|COFF_SectionFlag_Align1Bytes;
  T_Ok(t_write_def_obj("data.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "data_z", ".data$z", str8_lit("five"), .raw_flags = data_flags },
      { "data_a", ".data$a", str8_lit("three"), .raw_flags = data_flags },
      { "data_bbbbb", ".data$bbbbb", str8_lit("four"), .raw_flags = data_flags },
      { "data_empty", ".data$", str8_lit("two"), .raw_flags = data_flags },
      { "data", ".data", str8_lit("one"), .raw_flags = data_flags },
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\xC3"), .flags = "rx:code@1" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe data.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *data_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
  T_Ok(data_section);

  String8 data = str8_substr(exe, rng_1u64(data_section->foff, data_section->foff + data_section->vsize));
  String8 expected_data = str8_lit("onetwothreefourfive");
  T_Ok(str8_match(data, expected_data, 0));
}

TEST(flag_conf)
{
  COFF_SectionFlags my_sect0_flags = COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemExecute;
  COFF_SectionFlags my_sect1_flags = COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead|COFF_SectionFlag_MemWrite;
  T_Ok(t_write_def_obj("conf.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "a", ".mysect", str8_lit("one"), .raw_flags = my_sect0_flags },
      { "b", ".mysect", str8_lit("two"), .raw_flags = my_sect1_flags },
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\xC3"), .flags = "rx:code@1" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe conf.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeaderArray my_sects = coff_section_header_array_from_name(arena, string_table, section_table, pe.section_count, str8_lit(".mysect"));
  T_Ok(my_sects.count == 2);

  COFF_SectionHeader *my_sect0 = &my_sects.v[0];
  COFF_SectionHeader *my_sect1 = &my_sects.v[1];
  T_Ok(my_sect0->flags == my_sect0_flags);
  T_Ok(my_sect1->flags == my_sect1_flags);
}

TEST(invalid_bss)
{
  COFF_SectionFlags bss_flags = COFF_SectionFlag_CntInitializedData|COFF_SectionFlag_MemRead;
  String8 bss_data = str8_lit("Hello, World");
  T_Ok(t_write_def_obj("bss.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "bss", ".bss", bss_data, .raw_flags = bss_flags },
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\xC3"), .flags = "rx:code@1" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe bss.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *bss_sect = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".bss"));
  T_Ok(bss_sect != 0);
  T_Ok(bss_sect->vsize == 0xC);
  T_Ok(bss_sect->flags == bss_flags);
  String8 data = str8_substr(exe, rng_1u64(bss_sect->foff, bss_sect->foff + bss_sect->vsize));
  T_Ok(str8_match(data, bss_data, 0));
}

TEST(common_block)
{
  U8 a_data[6] = {0};
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "data", ".data", str8_array_fixed(a_data), .flags = "rw:data@1",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 0, "A"),
          {0}
        }
      },
      { "bss", ".bss", str8(0, 1), .flags = "rw:bss" }, // shift common block's initial position
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Common("A", 3),
      {0}
    }
  }));

  U8 b_data[9] = { 0 };
  T_Ok(t_write_def_obj("b.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "data", ".data", str8_array_fixed(b_data), .flags = "rw:data@1",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr64, 0, "B"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Common("B", 6),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\xC3"), .flags = "rx:code@1" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe /fixed /largeaddressaware:no /merge:.bss=.comm a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  COFF_SectionHeader *comm_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".comm"));
  COFF_SectionHeader *data_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
  T_Ok(comm_sect != 0);
  T_Ok(data_sect != 0);

  // blocks must be sorted in descending order to reduce alignment padding
  T_Ok(comm_sect->vsize == 0x13);

  // ensure linker correctly patched addresses for symbols pointing into common block
  String8             data      = str8_substr(exe, rng_1u64(data_sect->foff, data_sect->foff + data_sect->fsize));
  U32                *a_addr    = (U32 *)data.str;
  U64                *b_addr    = (U64 *)(data.str + sizeof(a_data));
  T_Ok(*a_addr == (pe.image_base + comm_sect->voff + 0x10));
  T_Ok(*b_addr == (pe.image_base + comm_sect->voff + 0x8));
}

TEST(base_relocs)
{
  // main.obj
  String8 entry_name = str8_lit("my_entry");
  U64 mov_func_name64 = 2;
  U64 mov_func_name32 = 16;
  U8 main_text[] = {
    0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // mov rax, func_name
    0xff, 0xd0,                                                  // call rax
    0x48, 0x31, 0xc0,                                            // xor rax, rax
    0xb8, 0x00, 0x00, 0x00, 0x00,                                // mov eax, func_name
    0xff, 0xd0,                                                  // call rax
    0xc3                                                         // ret
  };

  // func.obj
  String8 func_name   = str8_lit("foo");
  U8      func_text[] = { 0xc3 };

  // linker must not produce base relocations for absolute symbol
  T_Ok(t_write_def_obj("main.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text", str8_array_fixed(main_text), .flags = "rx:code@1",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr64, mov_func_name64, "foo"),
          T_COFF_DefReloc(X64_Addr32, mov_func_name32, "foo"),
          {0}
        }
      },
      {
        "data", ".data", str8_lit_comp("\x00\x00\x00\x00"), .flags = "rw:data",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32, 0, "abs"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("foo"),
      T_COFF_DefSymbol_AbsStatic("abs", 0x12345678),
      T_COFF_DefSymbol_Extern((char *)entry_name.str, "text", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("func.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_array_fixed(func_text), .flags = "rx:code@1" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern((char *)func_name.str, "text", 0),
      {0}
    }
  }));

  String8 out_name = str8_lit("a.exe");
  t_invoke_linkerf("/subsystem:console /entry:my_entry /dynamicbase /largeaddressaware:no /out:a.exe main.obj func.obj");
  T_Ok(g_last_exit_code == 0);

  // it is illegal to merge .reloc with other sections
  t_invoke_linkerf("/subsystem:console /entry:my_entry /dynamicbase /largeaddressaware:no /out:a.exe /merge:.reloc=.rdata main.obj func.obj");
  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_IllegalSectionMerge);
  } else {
    T_Ok(g_last_exit_code != 0);
  }

  // the other way around is illegal too
  t_invoke_linkerf("/subsystem:console /entry:my_entry /dynamicbase /largeaddressaware:no /out:a.exe /merge:.rdata=.reloc main.obj func.obj");
  if (t_id_linker() == T_Linker_RAD) {
    T_Ok(g_last_exit_code == LNK_Error_IllegalSectionMerge);
  } else {
    T_Ok(g_last_exit_code != 0);
  }
}

TEST(simple_lib_test)
{
  String8 test_payload = str8_lit("The quick brown fox jumps over the lazy dog");

  T_Ok(t_write_def_lib("test.lib", (T_COFF_DefLib){
    .emit_second_member = 1,
    .members = (T_COFF_DefLibMember[]){
      {
        .type = T_COFF_DefLibMember_Obj,
        .obj = {
          .path = str8_lit("test.obj"),
          .machine = T_COFF_DefSetMachine(Unknown),
          .sections = (T_COFF_DefSection[]){
            { "data", ".data", str8(test_payload.str, test_payload.size+1), .flags = "rw:data" },
            {0}
          },
          .symbols = (T_COFF_DefSymbol[]){
            T_COFF_DefSymbol_Extern("test", "data", 0),
            {0}
          }
        }
      },
      {0}
    }
  }));

  U8 entry_text[] = {
    0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00,
    0xC3
  };
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text", str8_array_fixed(entry_text), .flags = "rx:code@1",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 3, "test"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("test"),
      T_COFF_DefSymbol_Extern("my_entry", "text", 7),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe entry.obj test.lib");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *text_sect = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
  COFF_SectionHeader *data_sect = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));

  String8 text_data = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->fsize));
  String8 data_data = str8_substr(exe, rng_1u64(data_sect->foff, data_sect->foff + data_sect->fsize));

  // was test payload linked?
  String8 data_string = str8_cstring_capped(data_data.str, data_data.str + data_data.size);
  T_Ok(str8_match(data_string, test_payload, 0));

  // do we have enough bytes to read text?
  T_Ok(text_data.size >= sizeof(entry_text));

  // linker must pull-in test.obj and patch relocation for "test" symbol
  U32 *data_addr32nb = (U32 *)(text_data.str+3);
  T_Ok(*data_addr32nb == data_sect->voff);
}

TEST(import_export)
{
  // write objs
  T_Ok(t_write_def_obj("import.obj", (T_COFF_DefObj){
     .machine = T_COFF_DefSetMachine(X64),
     .sections = (T_COFF_DefSection[]){
       {
         .id    = "data",
         .name  = ".data",
         .flags = "rw:data",
         .data  = str8_array_fixed((U8[1024]){0}),
         .relocs = (T_COFF_DefReloc[]){
           T_COFF_DefReloc(X64_Addr32Nb, 0*4, "__imp_foo"),
           T_COFF_DefReloc(X64_Addr32Nb, 1*4, "__imp_bar"),
           T_COFF_DefReloc(X64_Addr32Nb, 2*4, "__imp_baz"),
           T_COFF_DefReloc(X64_Addr32Nb, 3*4, "__imp_baf"),
           T_COFF_DefReloc(X64_Addr32Nb, 4*4, "__imp_ord"),
           T_COFF_DefReloc(X64_Addr32Nb, 5*4, "bar"),
           T_COFF_DefReloc(X64_Addr32Nb, 6*4, "foo"),
           T_COFF_DefReloc(X64_Addr32Nb, 7*4, "ord"),
           {0},
         }
       },
       {0}
     },
     .symbols = (T_COFF_DefSymbol[]){
       T_COFF_DefSymbol_Undef("__imp_foo"),
       T_COFF_DefSymbol_Undef("__imp_bar"),
       T_COFF_DefSymbol_Undef("__imp_baz"),
       T_COFF_DefSymbol_Undef("__imp_baf"),
       T_COFF_DefSymbol_Undef("__imp_ord"),
       T_COFF_DefSymbol_Undef("bar"),
       T_COFF_DefSymbol_Undef("foo"),
       T_COFF_DefSymbol_Undef("ord"),
       //"baf",
       //"baz",
       //"__imp_ord2",
       //"__imp_ord4",
       {0},
     },
   }));

  T_Ok(t_write_def_obj("export.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "data", ".data", str8_lit("test"), .flags = "rw:data" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("foo", "data", 0),
      T_COFF_DefSymbol_Extern("ord", "data", 1),
      T_COFF_DefSymbol_Extern("ord2", "data", 2),
      T_COFF_DefSymbol_Extern("ord3", "data", 9),
      T_COFF_DefSymbol_Extern("ord4", "data", 10),
      {0}
    },
    .directives = (char*[]){
      "/export:foo=foo",
      "/export:bar=foo",
      "/export:ord,@5",
      "/export:ord2,@6,DATA",
      "/export:ord3,@7,NONAME,PRIVATE",
      "/export:ord4,@8,NONAME,DATA",
      "/export:baz=BAZ.qwe",
      "/export:baf=BAZ.#1",
      0,
    }
  }));

  T_Ok(t_write_def_obj("baz.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "s1", ".s1", str8_lit("s1"), .flags = "rw:data" },
      { "s2", ".s2", str8_lit("s2"), .flags = "rw:data" },
      { "text", ".text", str8_lit_comp("\xc3"), .flags = "rx:code" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("_DllMainCRTStartup", "text", 0),
      T_COFF_DefSymbol_Extern("s1", "s1", 0),
      T_COFF_DefSymbol_Extern("s2", "s2", 0),
      {0}
    },
    .directives = (char*[]){
      "/export:baf=s1",
      "/export:baz=s2",
      0,
    }
  }));

  T_Ok(t_write_entry_obj());
  
  // link dlls
  t_invoke_linkerf("/dll /out:export.dll libcmt.lib export.obj"); // export.dll
  T_Ok(g_last_exit_code == 0);
  t_invoke_linkerf("/dll /out:baz.dll /export:s1,@1,NONAME /export:qwe=s2 baz.obj"); // baz.dll
  T_Ok(g_last_exit_code == 0);

  // validate export table in export.dll
  if (t_id_linker() == T_Linker_RAD) {
    // validate export table in export.dll
    {
      String8              dll           = t_read_file(arena, str8_lit("export.dll"));
      PE_BinInfo           pe            = pe_bin_info_from_data(arena, dll);
      COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(dll, pe.section_table_range).str;
      PE_ParsedExportTable export_table  = pe_exports_from_data(arena, pe.section_count, section_table, dll, pe.data_dir_franges[PE_DataDirectoryIndex_EXPORT], pe.data_dir_vranges[PE_DataDirectoryIndex_EXPORT]);
      COFF_SectionHeader  *data_sect     = coff_section_header_from_name(str8_zero(), section_table, pe.section_count, str8_lit(".data"));

      // validate header
      T_Ok(export_table.flags == 0);
      T_Ok(export_table.time_stamp == COFF_TimeStamp_Max);
      T_Ok(export_table.major_ver == 0);
      T_Ok(export_table.minor_ver == 0);
      T_Ok(export_table.ordinal_base == 5);
      T_Ok(export_table.export_count == 8);

      // validate names
      T_Ok(str8_match(export_table.exports[0].name, str8_lit("baf"), 0));
      T_Ok(str8_match(export_table.exports[1].name, str8_lit("bar"), 0));
      T_Ok(str8_match(export_table.exports[2].name, str8_lit("baz"), 0));
      T_Ok(str8_match(export_table.exports[3].name, str8_lit("foo"), 0));
      T_Ok(str8_match(export_table.exports[4].name, str8_lit("ord"), 0));
      T_Ok(str8_match(export_table.exports[5].name, str8_lit("ord2"), 0));
      T_Ok(export_table.exports[6].name.size == 0);
      T_Ok(export_table.exports[7].name.size == 0);

      // validate forwarders
      T_Ok(str8_match(export_table.exports[0].forwarder, str8_lit("BAZ.#1"), 0));
      T_Ok(export_table.exports[1].forwarder.size == 0);
      T_Ok(str8_match(export_table.exports[2].forwarder, str8_lit("BAZ.qwe"), 0));
      T_Ok(export_table.exports[3].forwarder.size == 0);
      T_Ok(export_table.exports[4].forwarder.size == 0);
      T_Ok(export_table.exports[5].forwarder.size == 0);
      T_Ok(export_table.exports[6].forwarder.size == 0);
      T_Ok(export_table.exports[7].forwarder.size == 0);

      // validate voffs
      T_Ok(export_table.exports[1].voff == data_sect->voff + 0x0);
      T_Ok(export_table.exports[3].voff == data_sect->voff + 0x0);
      T_Ok(export_table.exports[4].voff == data_sect->voff + 0x1);
      T_Ok(export_table.exports[5].voff == data_sect->voff + 0x2);
      T_Ok(export_table.exports[6].voff == data_sect->voff + 0x9);
      T_Ok(export_table.exports[7].voff == data_sect->voff + 0xa);

      // validate ordinals
      T_Ok(export_table.exports[0].ordinal == 9);
      T_Ok(export_table.exports[1].ordinal == 10);
      T_Ok(export_table.exports[2].ordinal == 11);
      T_Ok(export_table.exports[3].ordinal == 12);
      T_Ok(export_table.exports[4].ordinal == 5);
      T_Ok(export_table.exports[5].ordinal == 6);
      T_Ok(export_table.exports[6].ordinal == 7);
      T_Ok(export_table.exports[7].ordinal == 8);
    }

    // validate export table in baz.dll
    {
      String8              dll           = t_read_file(arena, str8_lit("baz.dll"));
      PE_BinInfo           pe            = pe_bin_info_from_data(arena, dll);
      COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(dll, pe.section_table_range).str;
      PE_ParsedExportTable export_table  = pe_exports_from_data(arena, pe.section_count, section_table, dll, pe.data_dir_franges[PE_DataDirectoryIndex_EXPORT], pe.data_dir_vranges[PE_DataDirectoryIndex_EXPORT]);

      // validate header
      T_Ok(export_table.flags == 0);
      T_Ok(export_table.time_stamp == COFF_TimeStamp_Max);
      T_Ok(export_table.major_ver == 0);
      T_Ok(export_table.minor_ver == 0);
      T_Ok(export_table.ordinal_base == 1);
      T_Ok(export_table.export_count == 4);

      // validate names
      T_Ok(str8_match(export_table.exports[0].name, str8_lit("baf"), 0));
      T_Ok(str8_match(export_table.exports[1].name, str8_lit("baz"), 0));
      T_Ok(str8_match(export_table.exports[2].name, str8_lit("qwe"), 0));
      T_Ok(str8_match(export_table.exports[3].name, str8_zero(), 0));

      // validate forwarders
      T_Ok(str8_match(export_table.exports[0].forwarder, str8_zero(), 0));
      T_Ok(str8_match(export_table.exports[1].forwarder, str8_zero(), 0));
      T_Ok(str8_match(export_table.exports[2].forwarder, str8_zero(), 0));
      T_Ok(str8_match(export_table.exports[3].forwarder, str8_zero(), 0));

      // validate voffs
      T_Ok(export_table.exports[0].voff == 0x3000);
      T_Ok(export_table.exports[1].voff == 0x4000);
      T_Ok(export_table.exports[2].voff == 0x4000);
      T_Ok(export_table.exports[3].voff == 0x3000);

      // validate ordinals
      T_Ok(export_table.exports[0].ordinal == 2);
      T_Ok(export_table.exports[1].ordinal == 3);
      T_Ok(export_table.exports[2].ordinal == 4);
      T_Ok(export_table.exports[3].ordinal == 1);
    }

  }

  #if OS_WINDOWS
  {
    T_Ok(SetDllDirectoryA((LPCSTR)g_wdir.str));
    HANDLE export_dll = LoadLibrary("export.dll");
    T_Ok(export_dll);

    // test query by function name
    //T_Ok(GetProcAddress(export_dll, "baf"));
    T_Ok(GetProcAddress(export_dll, "bar"));
    //T_Ok(GetProcAddress(export_dll, "baz"));
    T_Ok(GetProcAddress(export_dll, "foo"));
    T_Ok(GetProcAddress(export_dll, "ord"));
    T_Ok(GetProcAddress(export_dll, "ord2"));

    // test query by ordinal
    //T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(9)));
    T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(10)));
    //T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(11)));
    T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(12)));
    T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(5)));
    T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(6)));
    T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(7)));
    T_Ok(GetProcAddress(export_dll, MAKEINTRESOURCE(8)));
  }
  #endif

  //T_Ok(t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /delayload:export.dll /export:entry kernel32.Lib delayimp.lib libcmt.lib export.lib import.obj entry.obj") == 0);
  // TODO: check import table
}

TEST(image_base)
{
  T_Ok(t_write_def_obj("image_base.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text",
        ".text",
        str8_lit_comp(
          "\x48\x8D\x0D\x00\x00\x00\x00"             // lea rcx, [__ImageBase]
          "\x48\xB8\x00\x00\x00\x00\x00\x00\x00\x00" // mov rax, __ImageBase
          "\xB8\x00\x00\x00\x00"                     // mov eax, __ImageBase
          "\xC3"                                     // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 3, "__ImageBase"),
          T_COFF_DefReloc(X64_Addr64, 9, "__ImageBase"),
          T_COFF_DefReloc(X64_Addr32Nb, 18, "__ImageBase"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("__ImageBase"),
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /base:0x2000000140000000 /out:a.exe image_base.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *text_section  = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
  T_Ok(text_section);

  U8 expected_text[] = {
    0x48, 0x8D, 0x0D, 0xF9, 0xEF, 0xFF, 0xFF,
    0x48, 0xB8, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x20,
    0xB8, 0x00, 0x00, 0x00, 0x00,
    0xC3
  };
  String8 text_data = str8_substr(exe, rng_1u64(text_section->foff, text_section->foff + sizeof(expected_text)));
  T_Ok(str8_match(text_data, str8_array_fixed(expected_text), 0));
}

TEST(comdat_any)
{
  T_Ok(t_write_def_obj("1.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "test", ".test$mn", str8_lit("1"), .flags = "rw:data@1", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("test", COFF_ComdatSelect_Any),
      T_COFF_DefSymbol_ExternFunc("TEST", "test", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("2.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "test", ".test$mn", str8_lit("2"), .flags = "rw:data@1", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("test", COFF_ComdatSelect_Any),
      T_COFF_DefSymbol_Extern("TEST", "test", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp("\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
                      "\xC3"), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("TEST"),
      {0}
    }
  }));

  {
    t_invoke_linkerf("/subsystem:console /entry:entry /out:1.exe 1.obj 2.obj entry.obj");
    T_Ok(g_last_exit_code == 0);
    String8             exe           = t_read_file(arena, str8_lit("1.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".test"));
    String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("1"), 0));
  }

  {
    t_invoke_linkerf("/subsystem:console /entry:entry /out:2.exe 2.obj 1.obj entry.obj");
    T_Ok(g_last_exit_code == 0);
    String8             exe           = t_read_file(arena, str8_lit("2.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".test"));
    String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("2"), 0));
  }
}

TEST(comdat_no_duplicates)
{
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "a"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("a"),
      {0}
    }
  }));

  String8 test_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "test", ".test", str8_lit("a"), .flags = "rw:data@1", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("test", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Extern("a", "test", 0),
      {0}
    }
  });
  T_Ok(t_write_file(str8_lit("a.obj"), test_obj));
  T_Ok(t_write_file(str8_lit("b.obj"), test_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) { T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol); }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe a.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("b.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".test"));
  T_Ok(sect);
  String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
  T_Ok(str8_match(data, str8_lit("a"), 0));
}

TEST(comdat_same_size)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "a", ".a", str8_lit("a"), .flags = "rw:data@1", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("a", COFF_ComdatSelect_SameSize),
      T_COFF_DefSymbol_Extern("TEST", "a", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("b.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "b", ".b", str8_lit("b"), .flags = "rw:data@1", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("b", COFF_ComdatSelect_SameSize),
      T_COFF_DefSymbol_Extern("TEST", "b", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("c.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "c", ".c", str8_lit("cc"), .flags = "rw:data@1", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("c", COFF_ComdatSelect_SameSize),
      T_COFF_DefSymbol_Extern("TEST", "c", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("TEST"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a"));
    T_Ok(sect != 0);
    String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("a"), 0));
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe a.obj b.obj c.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) { T_Ok(g_last_exit_code == LNK_Error_MultiplyDefinedSymbol); }
}

TEST(comdat_exact_match)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "a", ".a", str8_lit("a"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("a", COFF_ComdatSelect_ExactMatch),
      T_COFF_DefSymbol_Extern("TEST", "a", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("a2.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "a2", ".a2", str8_lit("a"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("a2", COFF_ComdatSelect_ExactMatch),
      T_COFF_DefSymbol_Extern("TEST", "a2", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("b.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "b", ".b", str8_lit("b"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("b", COFF_ComdatSelect_ExactMatch),
      T_COFF_DefSymbol_Extern("TEST", "b", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("TEST"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj b.obj");
  T_Ok(g_last_exit_code != 0);

  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe entry.obj a2.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("b.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a2"));
    T_Ok(sect != 0);
    String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("a"), 0));
  }
}

TEST(comdat_largest)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "a", ".a", str8_lit("a"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("a", COFF_ComdatSelect_Largest),
      T_COFF_DefSymbol_Extern("TEST", "a", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("b.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "b", ".b", str8_lit("bb"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("b", COFF_ComdatSelect_Largest),
      T_COFF_DefSymbol_Extern("TEST", "b", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("c.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "c", ".c", str8_lit("c"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("c", COFF_ComdatSelect_Largest),
      T_COFF_DefSymbol_Extern("TEST", "c", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("TEST"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /out:a.exe /entry:entry entry.obj a.obj b.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *discard_sect  = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a"));
    T_Ok(discard_sect == 0);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".b"));
    T_Ok(sect != 0);
    String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("bb"), 0));
  }

  t_invoke_linkerf("/subsystem:console /out:b.exe /entry:entry entry.obj c.obj a.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8             exe           = t_read_file(arena, str8_lit("b.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".c"));
    T_Ok(sect != 0);
    String8             data          = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));
    T_Ok(str8_match(data, str8_lit("c"), 0));
  }
}

TEST(comdat_associative)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "a", "a", str8_lit("a"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      { "aa", "aa", str8_lit("aa"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("a", COFF_ComdatSelect_Largest),
      T_COFF_DefSymbol_Extern("TEST", "a", 0),
      T_COFF_DefSymbol_Associative("aa", "a"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("b.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "bb", "bb", str8_lit("bb"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      { "b", "b", str8_lit("b"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      { "bbb", "bbb", str8_lit("bbb"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("bb", COFF_ComdatSelect_Largest),
      T_COFF_DefSymbol_Associative("b", "bb"),
      T_COFF_DefSymbol_Associative("bbb", "bb"),
      T_COFF_DefSymbol_Extern("TEST", "bb", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("TEST"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj b.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *a   = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit("a"));
  COFF_SectionHeader *aa  = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit("aa"));
  COFF_SectionHeader *b   = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit("b"));
  COFF_SectionHeader *bb  = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit("bb"));
  COFF_SectionHeader *bbb = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit("bbb"));
  T_Ok(a == 0);
  T_Ok(aa == 0);
  T_Ok(b != 0);
  T_Ok(bb != 0);
  T_Ok(bbb != 0);
  String8 b_data = str8_substr(exe, rng_1u64(b->foff, b->foff + b->vsize));
  String8 bb_data = str8_substr(exe, rng_1u64(bb->foff, bb->foff + bb->vsize));
  String8 bbb_data = str8_substr(exe, rng_1u64(bbb->foff, bbb->foff + bbb->vsize));
  T_Ok(str8_match(b_data, str8_lit("b"), 0));
  T_Ok(str8_match(bb_data, str8_lit("bb"), 0));
  T_Ok(str8_match(bbb_data, str8_lit("bbb"), 0));
}

TEST(comdat_associative_loop)
{
  T_Ok(t_write_def_obj("loop.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "aaaa", ".aaaa", str8_lit("aaaa"), .flags = "rw:data@1", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      { "aa",   ".aa",   str8_lit("aa"),   .flags = "rw:data@1", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      { "a",    ".a",    str8_lit("a"),    .flags = "rw:data@1", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      { "aaa",  ".aaa",  str8_lit("aaa"),  .flags = "rw:data@1", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Associative("aaa", "aa"),
      T_COFF_DefSymbol_Associative("aaaa", "aaa"),
      T_COFF_DefSymbol_Associative("a", "aa"),
      T_COFF_DefSymbol_Associative("aa", "a"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("TEST"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe loop.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) { T_Ok(g_last_exit_code == LNK_Error_AssociativeLoop); }
}

TEST(comdat_associative_non_comdat)
{
  T_Ok(t_write_def_obj("test.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "a", ".a", str8_lit("a"), .flags = "rw:data" },
      { "b", ".b", str8_lit("b"), .flags = "rw:data" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("TEST", "a", 0),
      T_COFF_DefSymbol_Associative("b", "a"),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("TEST"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj test.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *a             = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a"));
  COFF_SectionHeader *b             = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".b"));
  T_Ok(a != 0);
  T_Ok(b != 0);
  String8             a_data        = str8_substr(exe, rng_1u64(a->foff, a->foff + a->vsize));
  String8             b_data        = str8_substr(exe, rng_1u64(b->foff, b->foff + b->vsize));
  T_Ok(str8_match(a_data, str8_lit("a"), 0));
  T_Ok(str8_match(b_data, str8_lit("b"), 0));
}

TEST(comdat_associative_out_of_bounds)
{
  {
    String8 obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "a", ".a", str8_lit("a"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
        { "aa", ".aa", str8_lit("aa"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
        {0}
      },
      .symbols = (T_COFF_DefSymbol[]){
        T_COFF_DefSymbol_Secdef("a", COFF_ComdatSelect_Any),
        T_COFF_DefSymbol_Extern("TEST", "a", 0),
        T_COFF_DefSymbol_Associative("aa", "a"),
        {0}
      }
    });
    {
      COFF_FileHeaderInfo header = coff_file_header_info_from_data(obj);
      String8 string_table = str8_substr(obj, header.string_table_range);
      String8 symbol_table = str8_substr(obj, header.symbol_table_range);
      COFF_ParsedSymbol symbol = coff_parse_symbol(header, string_table, symbol_table, 3);
      AssertAlways(str8_match(symbol.name, str8_lit(".aa"), 0));
      AssertAlways(symbol.aux_symbol_count == 1);
      COFF_Symbol16 *symbol16 = symbol.raw_symbol;
      COFF_SymbolSecDef *secdef = (COFF_SymbolSecDef *)(symbol16 + 1);
      secdef->number_lo = 321;
    }
    T_Ok(t_write_file(str8_lit("bad.obj"), obj));
  }

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("TEST"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj bad.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) { T_Ok(g_last_exit_code == LNK_Error_IllData); }
}

TEST(comdat_with_offset)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "rdata", ".rdata", str8_lit_cstr("1Hello, World!"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("rdata", COFF_ComdatSelect_Largest),
      T_COFF_DefSymbol_Extern("TEST", "rdata", 1),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("b.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "rdata", ".rdata", str8_lit_cstr("Hello, World!"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("rdata", COFF_ComdatSelect_Largest),
      T_COFF_DefSymbol_Extern("TEST", "rdata", 1),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3" // ret
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 3, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("TEST"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == 0);
}

TEST(reloc_against_removed_comdat)
{
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "rdata", ".rdata", str8_lit_cstr("1Hello, World!"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("rdata", COFF_ComdatSelect_Largest),
      T_COFF_DefSymbol_Extern("TEST", "rdata", 1),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("b.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "comdat", ".rdata", str8_lit_cstr("H"), .flags = "r:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {
        "regular", ".rdata", str8_lit_comp("\x00\x00\x00\x00"), .flags = "r:data",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "STATIC"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("comdat", COFF_ComdatSelect_Largest),
      T_COFF_DefSymbol_Extern("TEST", "comdat", 1),
      T_COFF_DefSymbol_Static("STATIC", "comdat", 2),
      {0}
    }
  }));
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3"
        ), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 3, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("TEST"),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == LNK_Error_RelocationAgainstRemovedSection);
}

TEST(sect_align)
{
  T_Ok(t_write_def_obj("test.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "shift", ".a", str8_lit("q"), .flags = "rw:data" },
      { "none", ".a", str8_lit("abc"), .flags = "rw:data" },
      { "a1", ".a", str8_lit("wr"), .flags = "rw:data@1" },
      { "a2", ".a", str8_lit("e"), .flags = "rw:data@2" },
      { "a4", ".a", str8_lit("ttttt"), .flags = "rw:data@4" },
      { "a8", ".a", str8_lit("g"), .flags = "rw:data@8" },
      { "a16", ".a", str8_lit("o"), .flags = "rw:data@16" },
      { "a32", ".a", str8_lit("p"), .flags = "rw:data@32" },
      { "a64", ".a", str8_lit("f"), .flags = "rw:data@64" },
      { "a128", ".a", str8_lit("x"), .flags = "rw:data@128" },
      { "a256", ".a", str8_lit("c"), .flags = "rw:data@256" },
      { "a512", ".a", str8_lit("v"), .flags = "rw:data@512" },
      { "a1024", ".a", str8_lit("b"), .flags = "rw:data@1024" },
      { "a2048", ".a", str8_lit("n"), .flags = "rw:data@2048" },
      { "a4096", ".a", str8_lit("m"), .flags = "rw:data@4096" },
      { "a8192", ".a", str8_lit("z"), .flags = "rw:data@8192" },
      { "text", ".text", str8_lit_comp("\xC3"), .flags = "rx:code@1" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("my_entry", "text", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:my_entry /out:a.exe /align:8192 test.obj");

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *sect = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".a"));
  T_Ok(sect);
  String8 sect_data = str8_substr(exe, rng_1u64(sect->foff, sect->foff + sect->vsize));

  String8 shift = str8_substr(sect_data, rng_1u64(0, 1));
  T_Ok(str8_match(shift, str8_lit("q"), 0));
  String8 a_none = str8_substr(sect_data, rng_1u64(16, 16 + 3));
  T_Ok(str8_match(a_none, str8_lit("abc"), 0));
  String8 a_1 = str8_substr(sect_data, rng_1u64(19, 21));
  T_Ok(str8_match(a_1, str8_lit("wr"), 0));
  String8 a_2 = str8_substr(sect_data, rng_1u64(22, 23));
  T_Ok(str8_match(a_2, str8_lit("e"), 0));
  String8 a_4 = str8_substr(sect_data, rng_1u64(24, 29));
  T_Ok(str8_match(a_4, str8_lit("ttttt"), 0));
  String8 a_8 = str8_substr(sect_data, rng_1u64(32, 33));
  T_Ok(str8_match(a_8, str8_lit("g"), 0));
  String8 a_16 = str8_substr(sect_data, rng_1u64(48, 49));
  T_Ok(str8_match(a_16, str8_lit("o"), 0));
  String8 a_32 = str8_substr(sect_data, rng_1u64(64, 65));
  T_Ok(str8_match(a_32, str8_lit("p"), 0));
  String8 a_64 = str8_substr(sect_data, rng_1u64(128, 129));
  T_Ok(str8_match(a_64, str8_lit("f"), 0));
  String8 a_128 = str8_substr(sect_data, rng_1u64(256, 257));
  T_Ok(str8_match(a_128, str8_lit("x"), 0));
  String8 a_256 = str8_substr(sect_data, rng_1u64(512, 513));
  T_Ok(str8_match(a_256, str8_lit("c"), 0));
  String8 a_512 = str8_substr(sect_data, rng_1u64(1024, 1025));
  T_Ok(str8_match(a_512, str8_lit("v"), 0));
  String8 a_1024 = str8_substr(sect_data, rng_1u64(2048, 2049));
  T_Ok(str8_match(a_1024, str8_lit("b"), 0));
  String8 a_2048 = str8_substr(sect_data, rng_1u64(4096, 4097));
  T_Ok(str8_match(a_2048, str8_lit("n"), 0));
  String8 a_4096 = str8_substr(sect_data, rng_1u64(8192, 8193));
  T_Ok(str8_match(a_4096, str8_lit("m"), 0));
  String8 a_8192 = str8_substr(sect_data, rng_1u64(16384, 16385));
  T_Ok(str8_match(a_8192, str8_lit("z"), 0));
}

TEST(alt_name)
{
  T_Ok(t_write_def_obj("test.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "data", ".data", str8_lit("test"), .flags = "rw:data" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("test", "data", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("foo.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "data", ".data", str8_lit("foo"), .flags = "rw:data" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("foo", "data", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3"
        ), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "foo"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("foo"),
      {0}
    }
  }));

  // basic alternate name test
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /alternatename:foo=test test.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  // linker should not chase alt name links
  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe /alternatename:foo=bar /alternatename:bar=test test.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // alt name conflict
  t_invoke_linkerf("/subsystem:console /entry:entry /out:c.exe /alternatename:foo=test /alternatename:foo=qwe test.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // syntax error
  t_invoke_linkerf("/subsystem:console /entry:entry /out:d.exe /alternatename:foo foo.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
  
  // syntax error
  t_invoke_linkerf("/subsystem:console /entry:entry /out:e.exe /alternatename:foo-oof foo.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // syntax error
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /alternatename:foo=test=bar foo.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // syntax error
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /alternatename:foo= foo.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // syntax error
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /alternatename:= foo.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // syntax error
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /alternatename: foo.obj entry.obj");
  T_Ok(g_last_exit_code != 0);

  // TODO: check that RAD Linker prints these warnings

  // warn about alt name to self alt name?
  t_invoke_linkerf("/subsystem:console /entry:entry /out:f.exe /alternatename:foo=foo foo.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  // warn about alt name to unknown symbol?
  t_invoke_linkerf("/subsystem:console /entry:entry /out:g.exe /alternatename:qwe=ewq foo.obj entry.obj");
  T_Ok(g_last_exit_code == 0);
}

TEST(include)
{
  T_Ok(t_write_def_lib("include.lib", (T_COFF_DefLib){
    .emit_second_member = 1,
    .members = (T_COFF_DefLibMember[]){
      {
        .type = T_COFF_DefLibMember_Obj,
        .obj = {
          .path = str8_lit("include.obj"),
          .machine = T_COFF_DefSetMachine(X64),
          .sections = (T_COFF_DefSection[]){
            { "data", ".data", str8_lit("foo"), .flags = "rw:data" },
            {0}
          },
          .symbols = (T_COFF_DefSymbol[]){
            T_COFF_DefSymbol_Extern("foo", "data", 0),
            {0}
          }
        }
      },
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3"
        ), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "entry"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      {0}
    }
  }));

  // simple include test
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /include:foo entry.obj include.lib");
  T_Ok(g_last_exit_code == 0);

  // validate that linker pulled-in include.obj
  {
    String8             exe           = t_read_file(arena, str8_lit("a.exe"));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *foo_sect      = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
    T_Ok(foo_sect != 0);
    String8             foo_data      = str8_substr(exe, rng_1u64(foo_sect->foff, foo_sect->foff + foo_sect->vsize));
    T_Ok(str8_match(foo_data, str8_lit("foo"), 0));
  }

  // test unresolved include
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /include:ewq entry.obj");
  T_Ok(g_last_exit_code != 0);
  if (t_id_linker() == T_Linker_RAD) { T_Ok(g_last_exit_code == LNK_Error_UnresolvedSymbol); }
}

TEST(communal_var_vs_regular)
{
  T_Ok(t_write_def_obj("communal.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Common("TEST", 1),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("defn.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "data", ".data", str8_lit("test"), .flags = "rw:data" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("TEST", "data", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3"
        ), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("TEST"),
      {0}
    }
  }));

  // linker should replace communal TEST with .data TEST

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe communal.obj defn.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe defn.obj communal.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  char *exes[] = { "a.exe", "b.exe" };
  for EachElement(i, exes) {
    String8             exe           = t_read_file(arena, str8_cstring(exes[i]));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *data_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
    T_Ok(data_sect);
    String8             data          = str8_substr(exe, rng_1u64(data_sect->foff, data_sect->foff + data_sect->vsize));
    T_Ok(str8_match(data, str8_lit("test"), 0));
  }
}

TEST(communal_var_vs_regular_comdat)
{
  T_Ok(t_write_def_obj("communal.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Common("TEST", 1),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("large.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "data", ".data", str8_lit("test"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("data", COFF_ComdatSelect_Largest),
      T_COFF_DefSymbol_Extern("TEST", "data", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3"
        ), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("TEST"),
      {0}
    }
  }));

  // linker should replace communal TEST with .data TEST
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe communal.obj large.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe large.obj communal.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  char *exes[] = { "a.exe", "b.exe" };
  for EachElement(i, exes) {
    String8             exe           = t_read_file(arena, str8_cstring(exes[i]));
    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *data_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".data"));
    T_Ok(data_sect);
    String8             data          = str8_substr(exe, rng_1u64(data_sect->foff, data_sect->foff + data_sect->vsize));
    T_Ok(str8_match(data, str8_lit("test"), 0));
  }
}

TEST(import_kernel32)
{
  T_Ok(t_write_def_obj("import.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "data", ".data", str8_lit_cstr("test"), .flags = "rw:data" },
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\x83\xEC\x68"                           // sub  rsp,68h                        ; alloc space on stack
          "\xC7\x44\x24\x48\x18\x00\x00\x00"           // mov  dword ptr [rsp+48h],18h        ; SECURITY_ATTRIBUTES.nLength
          "\x48\xC7\x44\x24\x50\x00\x00\x00\x00"       // mov  qword ptr [rsp+50h],0          ; SECURITY_ATTRIBUTES.lpSecurityDescriptor
          "\xC7\x44\x24\x58\x00\x00\x00\x00"           // mov  dword ptr [rsp+58h],0          ; SECURITY_ATTRIBUTES.bInheritHandle
          "\x48\xC7\x44\x24\x30\x00\x00\x00\x00"       // mov  qword ptr [rsp+30h],0          ; hTemplateFile
          "\xC7\x44\x24\x28\x80\x00\x00\x00"           // mov  dword ptr [rsp+28h],80h        ; dwFlagsAndAttributes
          "\xC7\x44\x24\x20\x02\x00\x00\x00"           // mov  dword ptr [rsp+20h],2          ; dwCreationDisposition
          "\x4C\x8D\x4C\x24\x48"                       // lea  r9,[rsp+48h]                   ; lpSecurityAttributes
          "\x45\x33\xC0"                               // xor  r8d,r8d                        ; dwShareMode
          "\xBA\x00\x00\x00\x40"                       // mov  edx,40000000h                  ; dwDesiredAccess
          "\x48\x8D\x0D\x00\x00\x00\x00"               // lea  rcx,[test]                     ; lpFileName
          "\xFF\x15\x00\x00\x00\x00"                   // call qword ptr [__imp_CreateFileA]  ; call CreateFileA
          "\x48\x89\xC1"                               // mov  rcx,rax                        ; hObject
          "\xFF\x15\x00\x00\x00\x00"                   // call qword ptr [__imp_CloseHandle]  ; call CloseHandle
          "\x33\xC0"                                   // xor  eax,eax                        ; clear result
          "\x48\x83\xC4\x68"                           // add  rsp,68h                        ; dealloc stack
          "\xC3"                                       // ret                                 ; return
        ),
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 70, "test"),
          T_COFF_DefReloc(X64_Rel32, 76, "__imp_CreateFileA"),
          T_COFF_DefReloc(X64_Rel32, 85, "__imp_CloseHandle"),
          {0},
        }
      },
      {0},
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("test", "data", 0),
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      T_COFF_DefSymbol_Undef("__imp_CreateFileA"),
      T_COFF_DefSymbol_Undef("__imp_CloseHandle"),
      {0},
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /fixed import.obj kernel32.lib");
  T_Ok(g_last_exit_code == 0);

#if OS_WINDOWS
  {
    String8 test_file_path = push_str8f(arena, "%S/test", g_wdir);
    os_delete_file_at_path(test_file_path);

    OS_ProcessLaunchParams launch_opts = {0};
    launch_opts.inherit_env = 0;
    launch_opts.path = g_wdir;
    str8_list_pushf(arena, &launch_opts.cmd_line, "%S/a.exe", g_wdir);
    OS_Handle handle = os_process_launch(&launch_opts);
    AssertAlways(!os_handle_match(handle, os_handle_zero()));
    U64 exit_code = max_U64;
    os_process_join(handle, max_U64, &exit_code);
    T_Ok(exit_code == 0);
    T_Ok(os_file_path_exists(test_file_path));
  }
#endif
}

TEST(delay_import)
{
  {
    U8 return_0[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, // mov rax, 0
      0xc3 // ret
    };
    U8 return_1[] = {
      0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00, // mov rax, 1
      0xc3 // ret
    };
    U8 return_2[] = {
      0x48, 0xC7, 0xC0, 0x02, 0x00, 0x00, 0x00, // mov rax, 2
      0xc3 // ret
    };
    T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "return_0", ".text", str8_array_fixed(return_0), .flags = "rx:code@1" },
        { "return_1", ".text", str8_array_fixed(return_1), .flags = "rx:code@1" },
        { "return_2", ".text", str8_array_fixed(return_2), .flags = "rx:code@1" },
        {0}
      },
      .symbols = (T_COFF_DefSymbol[]){
        T_COFF_DefSymbol_ExternFunc("return_1", "return_1", 0),
        T_COFF_DefSymbol_ExternFunc("return_2", "return_2", 0),
        {0}
      }
    }));
  }

  {
    U8 return_0[] = {
      0x48, 0xC7, 0xC0, 0x00, 0x00, 0x00, 0x00, // mov rax, 0
      0xc3 // ret
    };
    U8 return_123[] = {
      0x48, 0xC7, 0xC0, 0x7B, 0x00, 0x00, 0x00, // mov rax, 123
      0xc3 // ret
    };
    U8 return_321[] = {
      0x48, 0xC7, 0xC0, 0x41, 0x01, 0x00, 0x00, // mov rax, 321 
      0xc3 // ret
    };
    T_Ok(t_write_def_obj("b.obj", (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "return_0", ".text", str8_array_fixed(return_0), .flags = "rx:code@1" },
        { "return_123", ".text", str8_array_fixed(return_123), .flags = "rx:code@1" },
        { "return_321", ".text", str8_array_fixed(return_321), .flags = "rx:code@1" },
        {0}
      },
      .symbols = (T_COFF_DefSymbol[]){
        T_COFF_DefSymbol_ExternFunc("return_123", "return_123", 0),
        T_COFF_DefSymbol_ExternFunc("return_321", "return_321", 0),
        {0}
      }
    }));
  }

  {
    U8 text[] = {
      0x56,                         // push  rsi
      0x57,                         // push  rdi
      0x48, 0x83, 0xEC, 0x28,       // sub   rsp,28h
      0xE8, 0x00, 0x00, 0x00, 0x00, // call  return_1
      0x89, 0xC6,                   // mov   esi,eax
      0xE8, 0x00, 0x00, 0x00, 0x00, // call  return_2
      0x89, 0xC7,                   // mov   edi,eax
      0x01, 0xF7,                   // add   edi,esi
      0xE8, 0x00, 0x00, 0x00, 0x00, // call  return_123
      0x89, 0xC6,                   // mov   esi,eax
      0xE8, 0x00, 0x00, 0x00, 0x00, // call  return_321
      0x01, 0xF0,                   // add   eax,esi
      0x01, 0xF8,                   // add   eax,edi
      0x48, 0x83, 0xC4, 0x28,       // add   rsp,28h
      0x5F,                         // pop   rdi
      0x5E,                         // pop   rsi
      0xC3,                         // ret
    };
    T_Ok(t_write_def_obj("main.obj", (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        {
          "text", ".text", str8_array_fixed(text), .flags = "rx:code@1",
          .relocs = (T_COFF_DefReloc[]){
            T_COFF_DefReloc(X64_Rel32, 7, "return_1"),
            T_COFF_DefReloc(X64_Rel32, 14, "return_2"),
            T_COFF_DefReloc(X64_Rel32, 23, "return_123"),
            T_COFF_DefReloc(X64_Rel32, 30, "return_321"),
            {0}
          }
        },
        {0}
      },
      .symbols = (T_COFF_DefSymbol[]){
        T_COFF_DefSymbol_Extern("entry", "text", 0),
        T_COFF_DefSymbol_Undef("return_1"),
        T_COFF_DefSymbol_Undef("return_2"),
        T_COFF_DefSymbol_Undef("return_123"),
        T_COFF_DefSymbol_Undef("return_321"),
        {0}
      }
    }));
  }

  t_invoke_linkerf("/dll /implib:a.lib /export:return_1 /export:return_2 /rad_time_stamp:0x69EB0E28 a.obj libcmt.lib");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/dll /implib:b.lib /export:return_123 /export:return_321 /rad_time_stamp:0x69EB0E28 b.obj libcmt.lib");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /fixed /debug:full /rad_time_stamp:0x69EB0E28 main.obj a.lib b.lib kernel32.lib delayimp.lib libcmt.lib /delayload:a.dll /delayload:b.dll");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  PE_ParsedDelayImportTable delay_import_table = pe_delay_imports_from_data(arena, pe.is_pe32, pe.section_count, section_table, exe, pe.data_dir_franges[PE_DataDirectoryIndex_DELAY_IMPORT]);

  PE_ParsedDelayDLLImport *a_import = &delay_import_table.v[0];
  T_Ok(a_import->attributes == 1);
  T_Ok(str8_match(a_import->name, str8_lit("a.dll"), 0));
  T_Ok(a_import->module_handle_voff != 0);
  T_Ok(a_import->name_table_voff != 0);
  T_Ok(a_import->bound_table_voff != 0);
  T_Ok(a_import->unload_table_voff != 0);
  T_Ok(a_import->time_stamp == 0);
  T_Ok(a_import->bound_table_count == 2);
  T_Ok(a_import->unload_table_count == 2);
  T_Ok(a_import->import_count == 2);

  PE_ParsedImport *return_1 = &a_import->imports[0];
  T_Ok(return_1->type == PE_ParsedImport_Name);
  T_Ok(str8_match(return_1->u.name.string, str8_lit("return_1"), 0));
  T_Ok(return_1->u.name.hint == 0);

  PE_ParsedImport *return_2 = &a_import->imports[1];
  T_Ok(return_2->type == PE_ParsedImport_Name);
  T_Ok(str8_match(return_2->u.name.string, str8_lit("return_2"), 0));
  T_Ok(return_2->u.name.hint == 1);

  PE_ParsedDelayDLLImport *b_import = &delay_import_table.v[1];
  T_Ok(b_import->attributes == 1);
  T_Ok(str8_match(b_import->name, str8_lit("b.dll"), 0));
  T_Ok(b_import->module_handle_voff != 0);
  T_Ok(b_import->name_table_voff != 0);
  T_Ok(b_import->bound_table_voff != 0);
  T_Ok(b_import->unload_table_voff != 0);
  T_Ok(b_import->time_stamp == 0);
  T_Ok(b_import->bound_table_count == 2);
  T_Ok(b_import->unload_table_count == 2);
  T_Ok(b_import->import_count == 2);

  PE_ParsedImport *return_123 = &b_import->imports[0];
  T_Ok(return_123->type == PE_ParsedImport_Name);
  T_Ok(str8_match(return_123->u.name.string, str8_lit("return_123"), 0));
  T_Ok(return_123->u.name.hint == 0);

  PE_ParsedImport *return_321 = &b_import->imports[1];
  T_Ok(return_321->type == PE_ParsedImport_Name);
  T_Ok(str8_match(return_321->u.name.string, str8_lit("return_321"), 0));
  T_Ok(return_321->u.name.hint == 1);
}

TEST(delay_import_user32)
{
  {
    U64 msg_off = 0;
    U64 caption_off = 5;
    String8 str_payload = str8_lit_comp("test\0foo\0");
    U8 text[] = {
      0x48, 0x83, 0xEC, 0x28,                   // sub  rsp,28h
      0x45, 0x33, 0xC9,                         // xor  r9d,r9d
      0x4C, 0x8D, 0x05, 0x00, 0x00, 0x00, 0x00, // lea  r8,[msg]
      0x48, 0x8D, 0x15, 0x00, 0x00, 0x00, 0x00, // lea  rdx,[caption]
      0x33, 0xC9,                               // xor  ecx,ecx
      0xFF, 0x15, 0x00, 0x00, 0x00, 0x00,       // call qword ptr [__imp_MessageBoxA]
      0x33, 0xC0,                               // xor  eax,eax
      0x48, 0x83, 0xC4, 0x28,                   // add  rsp,28h
      0xC3,                                     // ret
    };
    T_Ok(t_write_def_obj("delay_import.obj", (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "str", ".str", str_payload, .flags = "rw:data" },
        {
          "text", ".text", str8_array_fixed(text), .flags = "rx:code@1",
          .relocs = (T_COFF_DefReloc[]){
            T_COFF_DefReloc(X64_Rel32, 10, "msg"),
            T_COFF_DefReloc(X64_Rel32, 17, "caption"),
            T_COFF_DefReloc(X64_Rel32, 25, "__imp_MessageBoxA"),
            {0}
          }
        },
        {0}
      },
      .symbols = (T_COFF_DefSymbol[]){
        T_COFF_DefSymbol_Extern("msg", "str", msg_off),
        T_COFF_DefSymbol_Extern("caption", "str", caption_off),
        T_COFF_DefSymbol_Extern("entry", "text", 0),
        T_COFF_DefSymbol_Undef("__imp_MessageBoxA"),
        {0}
      }
    }));
  }

  t_invoke_linkerf("/subsystem:console /out:a.exe /entry:entry /fixed /delayload:user32.dll kernel32.lib user32.lib libcmt.lib delayimp.lib delay_import.obj /debug:full");
  T_Ok(g_last_exit_code == 0);
}

TEST(empty_section)
{
  T_Ok(t_write_def_obj("empty_section.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "test", ".test", str8(0,0), .flags = "rx:code" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("TEST", "test", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3"
        ), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 3, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("TEST"),
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe empty_section.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
}

TEST(removed_section)
{
  T_Ok(t_write_def_obj("test.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "test", ".test", str8_lit_comp("\xC3"), .flags = "rx:code", .raw_flags = COFF_SectionFlag_LnkRemove },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("TEST", "test", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3"
        ), // ret
        .flags = "rx:code@1",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 3, "TEST"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("TEST"),
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe test.obj entry.obj");
  T_Ok(g_last_exit_code != 0);
}

TEST(function_pad_min)
{
  T_Ok(t_write_def_obj("funcs.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "a", ".text", str8_lit_comp("\xC3"), .flags = "rx:code@4" },
      { "b", ".text", str8_lit_comp("\xC3"), .flags = "rx:code@4" },
      { "c", ".text", str8_lit_comp("\xC3"), .flags = "rx:code@1" },
      {0},
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_ExternFunc("A", "a", 0),
      T_COFF_DefSymbol_ExternFunc("B", "b", 0),
      T_COFF_DefSymbol_ExternFunc("C", "c", 0),
      {0},
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:A /functionpadmin:1 /out:a.exe funcs.obj");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *text_sect     = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));
  T_Ok(text_sect != 0);
  String8             text_data     = str8_substr(exe, rng_1u64(text_sect->foff, text_sect->foff + text_sect->vsize));

  U8 expected_text[] = {
    0xcc, 0xcc, 0xcc, 0xcc, 0xc3, 
    0xcc, 0xcc, 0xcc, 0xc3, 
    0xcc, 0xc3, 
  };
  T_Ok(str8_match(text_data, str8_array_fixed(expected_text), 0));
}

TEST(first_member_header)
{
  T_Ok(t_write_def_lib("test.lib", (T_COFF_DefLib){
    .members = (T_COFF_DefLibMember[]){
      {
        .type = T_COFF_DefLibMember_Obj,
        .obj = {
          .path = str8_lit("obj.obj"),
          .machine = T_COFF_DefSetMachine(X64),
          .symbols = (T_COFF_DefSymbol[]){
            T_COFF_DefSymbol_AbsExtern("8", 0x8),
            T_COFF_DefSymbol_AbsExtern("1", 0x1),
            T_COFF_DefSymbol_AbsExtern("9", 0x9),
            T_COFF_DefSymbol_AbsExtern("7", 0x7),
            T_COFF_DefSymbol_AbsExtern("4", 0x4),
            T_COFF_DefSymbol_AbsExtern("5", 0x5),
            T_COFF_DefSymbol_AbsExtern("2", 0x2),
            T_COFF_DefSymbol_AbsExtern("3", 0x3),
            T_COFF_DefSymbol_AbsExtern("6", 0x6),
            {0}
          }
        }
      },
      {0}
    }
  }));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe test.lib entry.obj /include:1 /include:2 /include:3 /include:4 /include:5 /include:6 /include:7 /include:8 /include:9");
  T_Ok(g_last_exit_code == 0);
}

TEST(second_member_header)
{
  T_Ok(t_write_def_lib("test.lib", (T_COFF_DefLib){
    .emit_second_member = 1,
    .members = (T_COFF_DefLibMember[]){
      {
        .type = T_COFF_DefLibMember_Obj,
        .obj = {
          .path = str8_lit("obj.obj"),
          .machine = T_COFF_DefSetMachine(X64),
          .symbols = (T_COFF_DefSymbol[]){
            T_COFF_DefSymbol_AbsExtern("8", 0x8),
            T_COFF_DefSymbol_AbsExtern("1", 0x1),
            T_COFF_DefSymbol_AbsExtern("9", 0x9),
            T_COFF_DefSymbol_AbsExtern("7", 0x7),
            T_COFF_DefSymbol_AbsExtern("4", 0x4),
            T_COFF_DefSymbol_AbsExtern("5", 0x5),
            T_COFF_DefSymbol_AbsExtern("2", 0x2),
            T_COFF_DefSymbol_AbsExtern("3", 0x3),
            T_COFF_DefSymbol_AbsExtern("6", 0x6),
            {0}
          }
        }
      },
      {0}
    }
  }));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe test.lib entry.obj /include:1 /include:2 /include:3 /include:4 /include:5 /include:6 /include:7 /include:8 /include:9");
  T_Ok(g_last_exit_code == 0);
}

#if 0
TEST(defer_imp_link)
{
  T_COFF_DefLib bar_lib_any = {
    .members = (T_COFF_DefLibMember[]){
      {
        .type = T_COFF_DefLibMember_DllImportStatic,
        .dll_import = { .name = "bar.dll" }
      },
      {
        .type = T_COFF_DefLibMember_Import,
        .import = { "bar.dll", "bar", COFF_ImportBy_Name, COFF_ImportHeader_Code, .hit_or_ordinal = 0 }
      },
      {
        .type = T_COFF_DefLibMember_Obj,
        .obj = {
          .machine  = T_COFF_DefSetMachine(X64),
          .sections = (T_COFF_DefSection[]){
            {
              "text", ".text", str8_lit_comp("\xff\x25\x00\x00\x00\x00"), .flags = "rx:code"
            },
            {0}
          },
          .symbols  = (T_COFF_DefSymbol[]){
            T_COFF_DefSymbol_Undef("__imp_bar"),
            T_COFF_DefSymbol_ExternFunc("qwe", "text", 0),
            {0},
          }
        }
      },
      {0}
    }
  };

  T_COFF_DefLib foo_lib_any = {
    .members = (T_COFF_DefLibMember[]){
      {
        .type = T_COFF_DefLibMember_DllImportStatic,
        .dll_import = { .name = "foo.dll" }
      },
      {
        .type = T_COFF_DefLibMember_Import,
        .import = { "foo.dll", "bar", COFF_ImportBy_Name, COFF_ImportHeader_Code, .hit_or_ordinal = 0 }
      },
      {
        .type = T_COFF_DefLibMember_Obj,
        .obj = {
          .machine  = T_COFF_DefSetMachine(X64),
          .sections = (T_COFF_DefSection[]){
            {
              "text", ".text",
              str8_lit_comp("\xff\x25\x00\x00\x00\x00"),
              .flags = "rx:code",
              .relocs = (T_COFF_DefReloc[]){
                T_COFF_DefReloc(X64_Rel32, 2, "bar"),
                {0}
              }
            },
            {0}
          },
          .symbols  = (T_COFF_DefSymbol[]){
            T_COFF_DefSymbol_Undef("bar"),
            T_COFF_DefSymbol_Undef("qwe"),
            T_COFF_DefSymbol_ExternFunc("thunk", "text", 0),
            {0},
          }
        }
      },
      {0}
    }
  };

  String8 bar_lib = t_coff_from_def_lib(arena, bar_lib_any);
  String8 foo_lib = t_coff_from_def_lib(arena, foo_lib_any);

  T_Ok(t_write_file(str8_lit("bar.lib"), bar_lib));
  T_Ok(t_write_file(str8_lit("foo.lib"), foo_lib));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe bar.lib foo.lib entry.obj /include:thunk");
  T_Ok(g_last_exit_code == 0);
}
#endif

TEST(defer_impl_link_to_second_search_pass)
{
  T_Ok(t_write_def_obj("imp_ref.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\xc3"), .flags = "rx:code" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_ExternFunc("entry", "text", 0),
      T_COFF_DefSymbol_Undef("__imp_foo"),
      T_COFF_DefSymbol_Undef("func"),
      {0}
    }
  }));

  T_Ok(t_write_def_lib("impl_ref.lib", (T_COFF_DefLib){
    .emit_second_member = 1,
    .members = (T_COFF_DefLibMember[]){
      {
        .type = T_COFF_DefLibMember_Obj,
        .obj = {
          .path = str8_lit("impl_ref.obj"),
          .machine = T_COFF_DefSetMachine(X64),
          .sections = (T_COFF_DefSection[]){
            { "text", ".text", str8_lit_comp("\xc3"), .flags = "rx:code" },
            {0}
          },
          .symbols = (T_COFF_DefSymbol[]){
            T_COFF_DefSymbol_ExternFunc("func", "text", 0),
            T_COFF_DefSymbol_Undef("foo"),
            {0}
          }
        }
      },
      {0}
    }
  }));

  T_Ok(t_write_def_lib("foo.lib", (T_COFF_DefLib){
    .emit_second_member = 1,
    .members = (T_COFF_DefLibMember[]){
      {
        .type = T_COFF_DefLibMember_Import,
        .import = {
          "foo.dll", "foo", COFF_ImportBy_Name, COFF_ImportHeader_Code,
          .hit_or_ordinal = 0,
          .time_stamp = T_COFF_DefSetTimeStamp(~0u),
          .machine = T_COFF_DefSetMachine(X64)
        }
      },
      {0}
    }
  }));

  T_Ok(t_write_def_lib("foo2.lib", (T_COFF_DefLib){
    .emit_second_member = 1,
    .members = (T_COFF_DefLibMember[]){
      {
        .type = T_COFF_DefLibMember_Import,
        .import = {
          "foo.dll", "foo", COFF_ImportBy_Name, COFF_ImportHeader_Code,
          .hit_or_ordinal = 0,
          .time_stamp = T_COFF_DefSetTimeStamp(~0u),
          .machine = T_COFF_DefSetMachine(X64)
        }
      },
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe foo.lib foo2.lib imp_ref.obj impl_ref.lib");
  T_Ok(g_last_exit_code == 0);
}

TEST(opt_ref_dangling_section)
{
  T_Ok(t_write_def_obj("entry.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "text", ".text",
        str8_lit_comp(
          "\x48\xC7\xC0\x00\x00\x00\x00" // mov rax, $imm
          "\xC3"
        ), // ret
        .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "f"),
          {0}
        }
      },
      {0},
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("f"),
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "data", ".data", str8_lit_cstr("A0000"), .flags = "rw:data", .raw_flags = COFF_SectionFlag_LnkCOMDAT,
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Addr32Nb, 0, "q"),
          {0}
        }
      },
      {0},
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Undef("q"),
      T_COFF_DefSymbol_Secdef("data", COFF_ComdatSelect_Largest),
      T_COFF_DefSymbol_Extern("f", "data", 0),
      {0}
    }
  }));

  T_Ok(t_write_def_lib("b.lib", (T_COFF_DefLib){
    .emit_second_member = 1,
    .members = (T_COFF_DefLibMember[]){
      {
        .type = T_COFF_DefLibMember_Obj,
        .obj = {
          .path = str8_lit("b.obj"),
          .machine = T_COFF_DefSetMachine(X64),
          .sections = (T_COFF_DefSection[]){
            { "q", ".q", str8_lit_comp("\x1\x2\x3\x4"), .flags = "rw:data" },
            {
              "data", ".data", str8_lit_cstr("BBBBBBBBBBBBBBB"),
              .flags = "rw:data",
              .raw_flags = COFF_SectionFlag_LnkCOMDAT
            },
            {0},
          },
          .symbols = (T_COFF_DefSymbol[]){
            T_COFF_DefSymbol_Extern("q", "q", 0),
            T_COFF_DefSymbol_Secdef("data", COFF_ComdatSelect_Largest),
            T_COFF_DefSymbol_Extern("f", "data", 0),
            {0},
          }
        }
      },
      {0},
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe entry.obj a.obj b.lib");
  T_Ok(g_last_exit_code == 0);

  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader *sect          = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".q"));
  T_Ok(sect != 0);
}

TEST(fail_if_mismatch)
{
  T_Ok(t_write_entry_obj());

  // ------------------------------------------------------------
  // try linking two objs with mismatching directives

  String8 a1 = t_make_obj_with_directive(arena, str8_lit("/FAILIFMISMATCH:a=1"));
  String8 a2 = t_make_obj_with_directive(arena, str8_lit("/FAILIFMISMATCH:a=2"));
  T_Ok(t_write_file(str8_lit("a1.obj"), a1));
  T_Ok(t_write_file(str8_lit("a2.obj"), a2));

  t_invoke_linkerf("entry.obj a1.obj a2.obj /entry:entry /subsystem:console /out:a2.exe");
  if (t_id_linker() == T_Linker_RAD) T_Ok(g_last_exit_code == LNK_Error_FailIfMismatch);
  else                               T_Ok(g_last_exit_code != 0);

  // ------------------------------------------------------------
  // happy case

  T_Ok(t_write_file(str8_lit("a1_copy.obj"), a1));

  t_invoke_linkerf("entry.obj a1.obj a1_copy.obj /entry:entry /subsystem:console /out:a1.exe");
  T_Ok(g_last_exit_code == 0);

  // ------------------------------------------------------------
  // test conflicting directives in obj

  String8 conf_dirs = t_make_obj_with_directive(arena, str8_lit("/FAILIFMISMATCH:a=1 /FAILIFMISMATCH:a=2"));
  T_Ok(t_write_file(str8_lit("conf_dirs.obj"), conf_dirs));

  t_invoke_linkerf("entry.obj conf_dirs.obj /entry:entry /subsystem:console /out:conf_dirs.exe");
  if (t_id_linker() == T_Linker_RAD) T_Ok(g_last_exit_code == LNK_Error_FailIfMismatch);
  else                               T_Ok(g_last_exit_code != 0);

  // ------------------------------------------------------------
  // passing switch on command line

  t_invoke_linkerf("entry.obj a1.obj /FAILIFMISMATCH:a=2 /out:cmddir.exe");
  if (t_id_linker() == T_Linker_RAD) T_Ok(g_last_exit_code == LNK_Error_FailIfMismatch);
  else                               T_Ok(g_last_exit_code != 0);
}

TEST(long_section_name)
{
  Arch arch = Arch_x64;

  T_Ok(t_write_def_obj("test.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit_comp("\xC3"), .flags = "rx:code" },
      { "debug_info", ".debug_info", str8_lit("DEBUG_INFO"), .flags = "rw:data" },
      { "debug_abbrev", ".debug_abbrev", str8_lit("DEBUG_ABBREV"), .flags = "rw:data" },
      {0},
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("entry", "text", 0),
      {0}
    }
  }));

  // link test.obj
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe test.obj");
  T_Ok(g_last_exit_code == 0);

  // load linked exe
  String8             exe           = t_read_file(arena, str8_lit("a.exe"));
  PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8             string_table  = str8_substr(exe, pe.string_table_range);

  COFF_SectionHeader *debug_info = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".debug_info"));
  T_Ok(debug_info);

  COFF_SectionHeader *debug_abbrev = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".debug_abbrev"));
  T_Ok(debug_abbrev);
}

TEST(debug_p_sig_mismatch)
{
  String8 a_obj_name = str8_lit("a.obj");
  String8 b_obj_name = str8_lit("b.obj");
  U32     a_sig      = 0xCAFEBABE;
  U32     b_sig      = 0xDEADBEEF;

  String8 a_debug_s;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);

    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    CV_C13SubSectionHeader *ss_header = str8_serial_push_size(arena, &srl, sizeof(*ss_header));
    U64 ss_start_off = srl.total_size;

    CV_SymObjName obj_name = {0};
    obj_name.sig = a_sig;
    String8 obj_name_string = a_obj_name;
    str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + sizeof(obj_name) + obj_name_string.size + 1);
    str8_serial_push_u16(arena, &srl, CV_SymKind_OBJNAME);
    str8_serial_push_struct(arena, &srl, &obj_name);
    str8_serial_push_cstr(arena, &srl, obj_name_string);
    str8_serial_push_align(arena, &srl, CV_SymbolAlign);

    String8 comp3_data = cv_make_comp3(arena,
                                       0,
                                       CV_Language_C,
                                       CV_Arch_X64,
                                       /* ver_fe_major */ 0,
                                       /* ver_fe_minor */ 0,
                                       /* ver_fe_build */ 0,
                                       /* ver_feqfe    */ 0,
                                       /* ver_major    */ 14,
                                       /* ver_minor    */ 36,
                                       /* ver_build    */ 32537,
                                       /* ver_qfe      */ 0,
                                       str8_lit(BUILD_TITLE));
    str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + comp3_data.size);
    str8_serial_push_u16(arena, &srl, CV_SymKind_COMPILE3);
    str8_serial_push_string(arena, &srl, comp3_data);
    str8_serial_push_align(arena, &srl, CV_SymbolAlign);

    ss_header->kind = CV_C13SubSectionKind_Symbols;
    ss_header->size = srl.total_size - ss_start_off;
    str8_serial_push_align(arena, &srl, CV_C13SubSectionAlign);

    a_debug_s = str8_serial_end(arena, &srl);
  }
  String8 a_debug_p;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);
    
    // signature
    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    // duplicate in a.obj
    CV_LeafPointer ptr = { .itype = CV_BasicType_VOID };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(ptr));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
    str8_serial_push_struct(arena, &srl, &ptr);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    // unique procedure type
    CV_LeafProcedure proc = { .ret_itype = 0x1000, .call_kind = CV_CallKind_NearPascal };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(proc));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_PROCEDURE);
    str8_serial_push_struct(arena, &srl, &proc);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    // PCH ender
    CV_LeafEndPreComp endprecomp = { .sig = a_sig };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(endprecomp));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_ENDPRECOMP);
    str8_serial_push_struct(arena, &srl, &endprecomp);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    a_debug_p = str8_serial_end(arena, &srl);
  }

  String8 b_debug_s;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);

    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    CV_C13SubSectionHeader *ss_header = str8_serial_push_size(arena, &srl, sizeof(*ss_header));
    U64 ss_start_off = srl.total_size;

    CV_SymObjName obj_name = {0};
    obj_name.sig = b_sig;
    String8 obj_name_string = a_obj_name;
    str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + sizeof(obj_name) + obj_name_string.size + 1);
    str8_serial_push_u16(arena, &srl, CV_SymKind_OBJNAME);
    str8_serial_push_struct(arena, &srl, &obj_name);
    str8_serial_push_cstr(arena, &srl, obj_name_string);
    str8_serial_push_align(arena, &srl, CV_SymbolAlign);

    String8 comp3_data = cv_make_comp3(arena,
                                       0,
                                       CV_Language_C,
                                       CV_Arch_X64,
                                       /* ver_fe_major */ 0,
                                       /* ver_fe_minor */ 0,
                                       /* ver_fe_build */ 0,
                                       /* ver_feqfe    */ 0,
                                       /* ver_major    */ 14,
                                       /* ver_minor    */ 36,
                                       /* ver_build    */ 32537,
                                       /* ver_qfe      */ 0,
                                       str8_lit(BUILD_TITLE));
    str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + comp3_data.size);
    str8_serial_push_u16(arena, &srl, CV_SymKind_COMPILE3);
    str8_serial_push_string(arena, &srl, comp3_data);
    str8_serial_push_align(arena, &srl, CV_SymbolAlign);

    ss_header->kind = CV_C13SubSectionKind_Symbols;
    ss_header->size = srl.total_size - ss_start_off;
    str8_serial_push_align(arena, &srl, CV_C13SubSectionAlign);

    b_debug_s = str8_serial_end(arena, &srl);
  }

  String8 b_debug_t;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);

    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    CV_LeafPreComp precomp = { .start_index = CV_MinComplexTypeIndex, .count = 2, sig = b_sig };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(precomp) + a_obj_name.size + 1);
    str8_serial_push_u16(arena, &srl, CV_LeafKind_PRECOMP);
    str8_serial_push_struct(arena, &srl, &precomp);
    str8_serial_push_cstr(arena, &srl, a_obj_name);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    CV_LeafPointer ptr = { .itype = CV_BasicType_VOID };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafPointer));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
    str8_serial_push_struct(arena, &srl, &ptr);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    CV_LeafProcedure proc = { .ret_itype = 0x1000, .call_kind = CV_CallKind_NearC };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafProcedure));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_PROCEDURE);
    str8_serial_push_struct(arena, &srl, &proc);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    b_debug_t = str8_serial_end(arena, &srl);
  }

  String8 a_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "debug_p", ".debug$P", a_debug_p, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
      { "debug_s", ".debug$S", a_debug_s, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
      {0}
    }
  });

  String8 b_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "debug_t", ".debug$T", b_debug_t, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
      { "debug_s", ".debug$S", b_debug_s, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
      {0}
    }
  });

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("b.obj"), b_obj));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /debug:full a.obj b.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  B32     found_error = 0;
  String8 a_obj_path    = t_make_file_path(arena, str8_lit("a.obj"));
  String8 b_obj_path    = t_make_file_path(arena, str8_lit("b.obj"));
  String8 expected_line = str8f(arena, "Error(%03d): %S: PCH signature mismatch, expected 0x%x got 0x%x; PCH obj %S", LNK_Error_PrecompSigMismatch, b_obj_path, b_sig, a_sig, a_obj_path);

  String8 output = g_errors;
  while (output.size) {
    String8 line = t_chop_line(&output);
    found_error = str8_match(line, expected_line, StringMatchFlag_CaseInsensitive);
    if (found_error) { break; }
  }
  T_Ok(found_error);
}

TEST(debug_p_and_debug_t_in_obj)
{
  U32     pch_sig      = 0xCAFEBABE;
  String8 pch_obj_name = str8_lit("pch.obj");

  String8 pch_debug_s;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);

    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    CV_C13SubSectionHeader *ss_header = str8_serial_push_size(arena, &srl, sizeof(*ss_header));
    U64 ss_start_off = srl.total_size;

    CV_SymObjName obj_name = {0};
    obj_name.sig = pch_sig;
    String8 obj_name_string = pch_obj_name;
    str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + sizeof(obj_name) + obj_name_string.size + 1);
    str8_serial_push_u16(arena, &srl, CV_SymKind_OBJNAME);
    str8_serial_push_struct(arena, &srl, &obj_name);
    str8_serial_push_cstr(arena, &srl, obj_name_string);
    str8_serial_push_align(arena, &srl, CV_SymbolAlign);

    ss_header->kind = CV_C13SubSectionKind_Symbols;
    ss_header->size = srl.total_size - ss_start_off;
    str8_serial_push_align(arena, &srl, CV_C13SubSectionAlign);

    pch_debug_s = str8_serial_end(arena, &srl);
  }

  String8 pch_debug_p;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);

    // signature
    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    CV_LeafPointer ptr = { .itype = CV_BasicType_VOID };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(ptr));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
    str8_serial_push_struct(arena, &srl, &ptr);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    // PCH ender
    CV_LeafEndPreComp endprecomp = { .sig = pch_sig };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(endprecomp));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_ENDPRECOMP);
    str8_serial_push_struct(arena, &srl, &endprecomp);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    pch_debug_p = str8_serial_end(arena, &srl);
  }

  String8 pch_debug_t;
  {
    String8List srl;
    str8_serial_begin(arena, &srl);

    CV_Signature sig = CV_Signature_C13;
    str8_serial_push_struct(arena, &srl, &sig);

    CV_LeafPreComp precomp = { .start_index = CV_MinComplexTypeIndex, .count = 1, sig = pch_sig };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(precomp) + pch_obj_name.size + 1);
    str8_serial_push_u16(arena, &srl, CV_LeafKind_PRECOMP);
    str8_serial_push_struct(arena, &srl, &precomp);
    str8_serial_push_cstr(arena, &srl, pch_obj_name);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    CV_LeafPointer ptr = { .itype = CV_BasicType_VOID };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafPointer));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
    str8_serial_push_struct(arena, &srl, &ptr);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    CV_LeafProcedure proc = { .ret_itype = 0x1000, .call_kind = CV_CallKind_NearC };
    str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafProcedure));
    str8_serial_push_u16(arena, &srl, CV_LeafKind_PROCEDURE);
    str8_serial_push_struct(arena, &srl, &proc);
    str8_serial_push_align(arena, &srl, CV_LeafAlign);

    pch_debug_t = str8_serial_end(arena, &srl);
  }

  T_Ok(t_write_def_obj("pch.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "debug_p", ".debug$P", pch_debug_p, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
      { "debug_t", ".debug$T", pch_debug_t, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
      { "debug_s", ".debug$S", pch_debug_s, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
      {0}
    }
  }));
  T_Ok(t_write_entry_obj());
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /debug:full pch.obj entry.obj");
  T_Ok(g_last_exit_code == 0);

  String8 output        = g_errors;
  B32     found_warning = 0;
  String8 pch_obj_path  = t_make_file_path(arena, str8_lit("pch.obj"));
  String8 expected_line = str8f(arena, "Warning(%03d): %S: multiple sections with debug types detected, obj must have either .debug$T or .debug$P; discarding both sections", LNK_Warning_MultipleDebugTAndDebugP, pch_obj_path);
  while (output.size) {
    String8 line = t_chop_line(&output);
    found_warning = str8_match(line, expected_line, StringMatchFlag_CaseInsensitive);
    if (found_warning) { break; }
  }
  T_Ok(found_warning);
}

TEST(merge_duplicate_types)
{
  {
    U32     pch_sig      = 0xCAFEBABE;
    String8 pch_obj_name = str8_lit("pch.obj");
    String8 a_obj_name   = str8_lit("a.obj");
    String8 c_obj_name   = str8_lit("c.obj");

    String8 pch_debug_s;
    {
      String8List srl;
      str8_serial_begin(arena, &srl);

      CV_Signature sig = CV_Signature_C13;
      str8_serial_push_struct(arena, &srl, &sig);

      CV_C13SubSectionHeader *ss_header = str8_serial_push_size(arena, &srl, sizeof(*ss_header));
      U64 ss_start_off = srl.total_size;

      CV_SymObjName obj_name = {0};
      obj_name.sig = pch_sig;
      String8 obj_name_string = pch_obj_name;
      str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + sizeof(obj_name) + obj_name_string.size + 1);
      str8_serial_push_u16(arena, &srl, CV_SymKind_OBJNAME);
      str8_serial_push_struct(arena, &srl, &obj_name);
      str8_serial_push_cstr(arena, &srl, obj_name_string);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      String8 comp3_data = cv_make_comp3(arena,
                                         0,
                                         CV_Language_C,
                                         CV_Arch_X64,
                                         /* ver_fe_major */ 0,
                                         /* ver_fe_minor */ 0,
                                         /* ver_fe_build */ 0,
                                         /* ver_feqfe    */ 0,
                                         /* ver_major    */ 14,
                                         /* ver_minor    */ 36,
                                         /* ver_build    */ 32537,
                                         /* ver_qfe      */ 0,
                                         str8_lit(BUILD_TITLE));
      str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + comp3_data.size);
      str8_serial_push_u16(arena, &srl, CV_SymKind_COMPILE3);
      str8_serial_push_string(arena, &srl, comp3_data);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      ss_header->kind = CV_C13SubSectionKind_Symbols;
      ss_header->size = srl.total_size - ss_start_off;
      str8_serial_push_align(arena, &srl, CV_C13SubSectionAlign);

      pch_debug_s = str8_serial_end(arena, &srl);
    }
    String8 debug_p;
    {
      String8List srl;
      str8_serial_begin(arena, &srl);
      
      // signature
      CV_Signature sig = CV_Signature_C13;
      str8_serial_push_struct(arena, &srl, &sig);

      // duplicate in a.obj
      CV_LeafPointer ptr = { .itype = CV_BasicType_VOID };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(ptr));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
      str8_serial_push_struct(arena, &srl, &ptr);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      // unique procedure type
      CV_LeafProcedure proc = { .ret_itype = 0x1000, .call_kind = CV_CallKind_NearPascal };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(proc));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_PROCEDURE);
      str8_serial_push_struct(arena, &srl, &proc);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      // PCH ender
      CV_LeafEndPreComp endprecomp = { .sig = pch_sig };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(endprecomp));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_ENDPRECOMP);
      str8_serial_push_struct(arena, &srl, &endprecomp);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      debug_p = str8_serial_end(arena, &srl);
    }

    String8 a_debug_s;
    {
      String8List srl;
      str8_serial_begin(arena, &srl);

      CV_Signature sig = CV_Signature_C13;
      str8_serial_push_struct(arena, &srl, &sig);

      CV_C13SubSectionHeader *ss_header = str8_serial_push_size(arena, &srl, sizeof(*ss_header));
      U64 ss_start_off = srl.total_size;

      CV_SymObjName obj_name = {0};
      obj_name.sig = pch_sig;
      String8 obj_name_string = a_obj_name;
      str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + sizeof(obj_name) + obj_name_string.size + 1);
      str8_serial_push_u16(arena, &srl, CV_SymKind_OBJNAME);
      str8_serial_push_struct(arena, &srl, &obj_name);
      str8_serial_push_cstr(arena, &srl, obj_name_string);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      String8 comp3_data = cv_make_comp3(arena,
                                         0,
                                         CV_Language_C,
                                         CV_Arch_X64,
                                         /* ver_fe_major */ 0,
                                         /* ver_fe_minor */ 0,
                                         /* ver_fe_build */ 0,
                                         /* ver_feqfe    */ 0,
                                         /* ver_major    */ 14,
                                         /* ver_minor    */ 36,
                                         /* ver_build    */ 32537,
                                         /* ver_qfe      */ 0,
                                         str8_lit(BUILD_TITLE));
      str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + comp3_data.size);
      str8_serial_push_u16(arena, &srl, CV_SymKind_COMPILE3);
      str8_serial_push_string(arena, &srl, comp3_data);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      ss_header->kind = CV_C13SubSectionKind_Symbols;
      ss_header->size = srl.total_size - ss_start_off;
      str8_serial_push_align(arena, &srl, CV_C13SubSectionAlign);

      a_debug_s = str8_serial_end(arena, &srl);
    }

    String8 debug_t;
    {
      String8List srl;
      str8_serial_begin(arena, &srl);

      CV_Signature sig = CV_Signature_C13;
      str8_serial_push_struct(arena, &srl, &sig);

      CV_LeafPreComp precomp = { .start_index = CV_MinComplexTypeIndex, .count = 2, sig = pch_sig };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(precomp) + pch_obj_name.size + 1);
      str8_serial_push_u16(arena, &srl, CV_LeafKind_PRECOMP);
      str8_serial_push_struct(arena, &srl, &precomp);
      str8_serial_push_cstr(arena, &srl, pch_obj_name);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      CV_LeafPointer ptr = { .itype = CV_BasicType_VOID };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafPointer));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
      str8_serial_push_struct(arena, &srl, &ptr);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      CV_LeafProcedure proc = { .ret_itype = 0x1000, .call_kind = CV_CallKind_NearC };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafProcedure));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_PROCEDURE);
      str8_serial_push_struct(arena, &srl, &proc);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      debug_t = str8_serial_end(arena, &srl);
    }

    String8 c_debug_t;
    {
      String8List srl;
      str8_serial_begin(arena, &srl);

      CV_Signature sig = CV_Signature_C13;
      str8_serial_push_struct(arena, &srl, &sig);

      CV_LeafPointer ptr = { .itype = CV_BasicType_SHORT };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafPointer));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_POINTER);
      str8_serial_push_struct(arena, &srl, &ptr);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      CV_LeafProcedure proc = { .ret_itype = 0x1000, .call_kind = CV_CallKind_NearC };
      str8_serial_push_u16(arena, &srl, sizeof(CV_LeafKind) + sizeof(CV_LeafProcedure));
      str8_serial_push_u16(arena, &srl, CV_LeafKind_PROCEDURE);
      str8_serial_push_struct(arena, &srl, &proc);
      str8_serial_push_align(arena, &srl, CV_LeafAlign);

      c_debug_t = str8_serial_end(arena, &srl);
    }

    String8 c_debug_s;
    {
      String8List srl;
      str8_serial_begin(arena, &srl);

      CV_Signature sig = CV_Signature_C13;
      str8_serial_push_struct(arena, &srl, &sig);

      CV_C13SubSectionHeader *ss_header = str8_serial_push_size(arena, &srl, sizeof(*ss_header));
      U64 ss_start_off = srl.total_size;

      // S_OBJNAME
      CV_SymObjName obj_name = {0};
      obj_name.sig = pch_sig;
      String8 obj_name_string = c_obj_name;
      str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + sizeof(obj_name) + obj_name_string.size + 1);
      str8_serial_push_u16(arena, &srl, CV_SymKind_OBJNAME);
      str8_serial_push_struct(arena, &srl, &obj_name);
      str8_serial_push_cstr(arena, &srl, obj_name_string);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      // S_COMPILE3
      String8 comp3_data = cv_make_comp3(arena,
                                         0,
                                         CV_Language_C,
                                         CV_Arch_X64,
                                         /* ver_fe_major */ 0,
                                         /* ver_fe_minor */ 0,
                                         /* ver_fe_build */ 0,
                                         /* ver_feqfe    */ 0,
                                         /* ver_major    */ 14,
                                         /* ver_minor    */ 36,
                                         /* ver_build    */ 32537,
                                         /* ver_qfe      */ 0,
                                         str8_lit(BUILD_TITLE));
      str8_serial_push_u16(arena, &srl, sizeof(CV_SymKind) + comp3_data.size);
      str8_serial_push_u16(arena, &srl, CV_SymKind_COMPILE3);
      str8_serial_push_string(arena, &srl, comp3_data);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      // S_LPROC32
      U16 *foo_size = str8_serial_push_size(arena, &srl, sizeof(*foo_size));
      U64  foo_off  = srl.total_size;
      str8_serial_push_u16(arena, &srl, CV_SymKind_LPROC32);
      CV_SymProc32 *foo_proc = str8_serial_push_size(arena, &srl, sizeof(*foo_proc));
      foo_proc->itype = 0x1001;
      foo_proc->sec = 1;
      foo_proc->len = 1;
      str8_serial_push_cstr(arena, &srl, str8_lit("foo"));
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);
      *foo_size = srl.total_size - foo_off;

      // S_PROC_ID_END
      str8_serial_push_u16(arena, &srl, 2);
      str8_serial_push_u16(arena, &srl, CV_SymKind_END);
      str8_serial_push_align(arena, &srl, CV_SymbolAlign);

      // $$Symbols header
      ss_header->kind = CV_C13SubSectionKind_Symbols;
      ss_header->size = srl.total_size - ss_start_off;
      str8_serial_push_align(arena, &srl, CV_C13SubSectionAlign);

      c_debug_s = str8_serial_end(arena, &srl);
    }

    String8 pch_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "debug_p", ".debug$P", debug_p, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
        { "debug_s", ".debug$S", pch_debug_s, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
        {0}
      }
    });

    String8 a_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "debug_t", ".debug$T", debug_t, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
        { "debug_s", ".debug$S", a_debug_s, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
        {0}
      }
    });

    String8 b_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "debug_t", ".debug$T", debug_t, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
        {0}
      }
    });

    String8 c_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "text", ".text", str8_lit_comp(""), .flags = "rx:code" },
        { "debug_t", ".debug$T", c_debug_t, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
        { "debug_s", ".debug$S", c_debug_s, .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
        {0}
      }
    });

    String8 entry_obj = t_make_entry_obj(arena);

    T_Ok(t_write_file(str8_lit("entry.obj"), entry_obj));
    T_Ok(t_write_file(str8_lit("pch.obj"), pch_obj));
    T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
    T_Ok(t_write_file(str8_lit("b.obj"), b_obj));
    T_Ok(t_write_file(str8_lit("c.obj"), c_obj));

    t_invoke_linkerf("/subsystem:console /entry:entry /debug:full /out:a.exe pch.obj a.obj b.obj c.obj entry.obj");
    T_Ok(g_last_exit_code == 0);
  }

  // load msf
  String8     pdb = t_read_file(arena, str8_lit("a.pdb"));
  MSF_Parsed *msf = msf_parsed_from_data(arena, pdb);
  // find named streams
  String8                info_data     = msf_data_from_stream(msf, PDB_FixedStream_Info);
  PDB_Info              *pdb_info      = pdb_info_from_data(arena, info_data);
  PDB_NamedStreamTable  *named_streams = pdb_named_stream_table_from_info(arena, pdb_info);
  // find string table
  MSF_StreamNumber  strtbl_sn   = named_streams->sn[PDB_NamedStream_StringTable];
  String8           strtbl_data = msf_data_from_stream(msf, strtbl_sn);
  PDB_Strtbl       *strtbl      = pdb_strtbl_from_data(arena, strtbl_data);
  // find TPI
  String8       tpi_data = msf_data_from_stream(msf, PDB_FixedStream_Tpi);
  PDB_TpiParsed *tpi     = pdb_tpi_from_data(arena, tpi_data);

  U64 type_count = tpi->itype_opl - tpi->itype_first;
  T_Ok(type_count == 5);
  
  CV_DebugT debug_t = cv_debug_t_from_data(arena, pdb_leaf_data_from_tpi(tpi), 4);
  T_Ok(debug_t.count == type_count);

  {
    CV_Leaf ptr_leaf  = cv_debug_t_get_leaf(&debug_t, 0);
    T_Ok(ptr_leaf.kind == CV_LeafKind_POINTER);
    T_Ok(ptr_leaf.data.size == sizeof(CV_LeafPointer));

    CV_LeafPointer *ptr = (CV_LeafPointer *)ptr_leaf.data.str;
    T_Ok(ptr->itype == CV_BasicType_VOID);
    T_Ok(ptr->attribs == 0);
  }

  {
    CV_Leaf proc_leaf = cv_debug_t_get_leaf(&debug_t, 1);
    T_Ok(proc_leaf.kind == CV_LeafKind_PROCEDURE);
    T_Ok(proc_leaf.data.size == sizeof(CV_LeafProcedure));

    CV_LeafProcedure *proc = (CV_LeafProcedure *)proc_leaf.data.str;
    T_Ok(proc->ret_itype == 0x1000);
    T_Ok(proc->call_kind == CV_CallKind_NearPascal);
  }

  {
    CV_Leaf proc_leaf = cv_debug_t_get_leaf(&debug_t, 2);
    T_Ok(proc_leaf.kind == CV_LeafKind_PROCEDURE);
    T_Ok(proc_leaf.data.size == sizeof(CV_LeafProcedure));

    CV_LeafProcedure *proc = (CV_LeafProcedure *)proc_leaf.data.str;
    T_Ok(proc->ret_itype == 0x1000);
    T_Ok(proc->call_kind == CV_CallKind_NearC);
  }

  {
    CV_Leaf ptr_leaf  = cv_debug_t_get_leaf(&debug_t, 3);
    T_Ok(ptr_leaf.kind == CV_LeafKind_POINTER);
    T_Ok(ptr_leaf.data.size == sizeof(CV_LeafPointer));

    CV_LeafPointer *ptr = (CV_LeafPointer *)ptr_leaf.data.str;
    T_Ok(ptr->itype == CV_BasicType_SHORT);
    T_Ok(ptr->attribs == 0);
  }

  {
    CV_Leaf proc_leaf = cv_debug_t_get_leaf(&debug_t, 4);
    T_Ok(proc_leaf.kind == CV_LeafKind_PROCEDURE);
    T_Ok(proc_leaf.data.size == sizeof(CV_LeafProcedure));

    CV_LeafProcedure *proc = (CV_LeafProcedure *)proc_leaf.data.str;
    T_Ok(proc->ret_itype == 0x1003);
    T_Ok(proc->call_kind == CV_CallKind_NearC);
  }
}

TEST(cyclic_type)
{
  String8List *debug_t = push_array(arena, String8List, 1);
  str8_serial_begin(arena, debug_t);
  str8_serial_push_u32(arena, debug_t, CV_Signature_C13);
  str8_serial_push_string(arena, debug_t, cv_make_leaf(arena, CV_LeafKind_POINTER, str8_struct((&(CV_LeafPointer){ .itype = 0x1001 })), CV_LeafAlign));
  str8_serial_push_string(arena, debug_t, cv_make_leaf(arena, CV_LeafKind_POINTER, str8_struct((&(CV_LeafPointer){ .itype = 0x1000 })), CV_LeafAlign));

  CV_DebugS debug_s = {0};
  str8_list_push(arena, &debug_s.data_list[CV_C13SubSectionIdxKind_Symbols], cv_make_symbol(arena, CV_SymKind_GPROC32, cv_make_proc32(arena, (CV_SymProc32){ .itype = 0x1001 }, str8_lit("foo"))));
  String8List debug_s_string = cv_data_from_debug_s_c13(arena, &debug_s, 1);

  String8 raw_coff = t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "debug_t", ".debug$T", str8_serial_end(arena, debug_t), .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
      { "debug_s", ".debug$S", str8_list_join(arena, &debug_s_string, 0), .flags = "r:data@1", .raw_flags = COFF_SectionFlag_MemDiscardable },
      {0}
    }
  });

  T_Ok(t_write_entry_obj());
  T_Ok(t_write_file(str8_lit("cycle.obj"), raw_coff));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /debug:full /rad_ignore:-%u cycle.obj entry.obj", LNK_Error_InvalidTypeIndex);
  T_Ok(g_last_exit_code == 0);

  String8 output            = g_errors;
  B32     is_cycle_detected = 0;
  while (output.size && !is_cycle_detected) {
    is_cycle_detected = t_match_linef(&output,
                      "Error(043): %S: LF_POINTER(type_index: 0x1000) forward refs member type index 0x1001 (leaf struct offset: 0x0)",
                      t_make_file_path(arena, str8_lit("cycle.obj")));
    t_chop_line(&output);
  }
  T_Ok(is_cycle_detected);
}

TEST(get_msf_stream_pages)
{
  MSF_Context *msf = msf_alloc(MSF_DEFAULT_PAGE_SIZE, MSF_DEFAULT_FPM);

  {
    U64 stream_size = MB(150) + 1;

    MSF_StreamNumber sn = msf_stream_alloc_ex(msf, stream_size);

    U8 *test = push_array(arena, U8, stream_size);
    MemorySet(test, 0xca, stream_size/2);
    MemorySet(test + stream_size/2, 0xbe, stream_size/2);

    String8List stream_data = msf_data_from_sn(arena, msf, sn);
    T_Ok(stream_data.total_size == stream_size);
    T_Ok(stream_data.node_count == 12);

    String8Array a = str8_array_from_list(arena, &stream_data);
    T_Ok(a.v[0].size  == 0xffd000);
    T_Ok(a.v[1].size  == 0xffe000);
    T_Ok(a.v[2].size  == 0xffe000);
    T_Ok(a.v[3].size  == 0xffe000);
    T_Ok(a.v[4].size  == 0xffe000);
    T_Ok(a.v[5].size  == 0xffe000);
    T_Ok(a.v[6].size  == 0xffe000);
    T_Ok(a.v[7].size  == 0xffd000);
    T_Ok(a.v[8].size  == 0x1000);
    T_Ok(a.v[9].size  == 0xffe000);
    T_Ok(a.v[10].size == 0x613000);
    T_Ok(a.v[11].size == 1);

    String8Node buf     = *stream_data.first;
    U64         buf_pos = 0;
    str8_buffer_write(&buf, &buf_pos, str8(test, stream_size));

    String8 cmp = msf_stream_read_block(arena, msf, sn, stream_size);
    T_Ok(cmp.size == stream_size);
    T_Ok(MemoryCompare(cmp.str, test, stream_size) == 0);
  }

  {
    MSF_StreamNumber sn = msf_stream_alloc_ex(msf, 1);
    String8List stream_data = msf_data_from_sn(arena, msf, sn);
    T_Ok(stream_data.node_count == 1);
    T_Ok(stream_data.total_size == 1);
    T_Ok(stream_data.first->string.size == 1);
  }

  msf_release(msf);
}

internal String8
data_from_pdb(Arena *arena, PDB_Context *pdb)
{
  TP_Context *tp       = tp_alloc(arena, 1, 1, str8_lit("foo"));
  TP_Arena   *tp_arena = tp_arena_alloc(tp);
  pdb_build(tp, tp_arena, pdb, (CV_StringHashTable){0}, 1, 0);

  AssertAlways(msf_build(pdb->msf) == MSF_Error_OK);
  String8List raw_msf_list = msf_get_page_data_nodes(arena, pdb->msf);
  AssertAlways(t_write_file_list(str8_lit("test.pdb"), raw_msf_list));

  String8 data = str8_list_join(arena, &raw_msf_list, 0);

  tp_arena_release(&tp_arena);
  tp_release(tp);

  return data;
}

TEST(validate_info_stream)
{
  COFF_TimeStamp  time_stamp = 123;
  U32             age        = 1;
  Guid            guid       = { .data1 = max_U32, .data2 = max_U16 - 1, .data3 = max_U16 - 2, .data4 = { 1, 2, 3, 4, 5, 6, 7, 8 } };
  PDB_Context    *pdb        = pdb_alloc(MSF_DEFAULT_PAGE_SIZE, COFF_MachineType_X64, time_stamp, age, guid);

  char *stream_names[] = { "one", "two", "three", "four", "five" };
  MSF_StreamNumber stream_numbers[ArrayCount(stream_names)] = {0};

  for EachElement(i, stream_names) {
    stream_numbers[i] = pdb_push_named_stream(&pdb->info->named_stream_ht, pdb->msf, str8_cstring(stream_names[i]));
    T_Ok(stream_numbers[i] != MSF_INVALID_STREAM_NUMBER);
  }
  
  String8     raw_msf    = data_from_pdb(arena, pdb);
  MSF_Parsed *msf_parsed = msf_parsed_from_data(arena, raw_msf);
  String8     info_data  = msf_data_from_stream(msf_parsed, PDB_FixedStream_Info);

#if 0
  fprintf(stderr, "\n");
  for EachIndex(i, info_data.size) {
    fprintf(stderr, "0x%02x, ", info_data.str[i]);
    if (i % 19 == 18 && i > 0) { fprintf(stderr, "\n"); }
  }
#endif
  U8 expected_info_data[] = {
    0x94, 0x2e, 0x31, 0x01, 0x7b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xfd,
    0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x22, 0x00, 0x00, 0x00, 0x6f, 0x6e, 0x65, 0x00, 0x74, 0x77,
    0x6f, 0x00, 0x74, 0x68, 0x72, 0x65, 0x65, 0x00, 0x66, 0x6f, 0x75, 0x72, 0x00, 0x66, 0x69, 0x76, 0x65, 0x00, 0x2f,
    0x4c, 0x69, 0x6e, 0x6b, 0x49, 0x6e, 0x66, 0x6f, 0x00, 0x06, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0xb7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x13,
    0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x51, 0x33, 0x01,
  };
  T_Ok(str8_match(info_data, str8_array_fixed(expected_info_data), 0));

  pdb_release(pdb);
}

TEST(validate_gsi)
{
  String8 raw_symbols[] = {
    cv_make_symbol(arena, CV_SymKind_CONSTANT, cv_make_const(arena, (CV_SymConstant){ .itype = 0 }, 263, str8_lit("CV_SymKind_BLOCK16"))),
    cv_make_symbol(arena, CV_SymKind_GDATA32, cv_make_data32(arena, (CV_SymData32)  { .itype = 0, .off = 0x25440, .sec = 2 }, str8_lit("__newclmap"))),
    cv_make_symbol(arena, CV_SymKind_GDATA32, cv_make_data32(arena, (CV_SymData32)  { .itype = 0, .off = 123,     .sec = 1 }, str8_lit("coffeebabe"))),
    cv_make_symbol(arena, CV_SymKind_GDATA32, cv_make_data32(arena, (CV_SymData32)  { .itype = 0, .off = 123,     .sec = 1 }, str8_lit("deadbeef"))),
  };

  CV_Symbol symbols[ArrayCount(raw_symbols)] = {0};
  for EachElement(i, raw_symbols) { symbols[i] = cv_symbol_from_ptr(raw_symbols[i].str); }

  CV_DebugS debug_s = {0};
  for EachElement(i, symbols) { str8_list_push(arena, &debug_s.data_list[CV_C13SubSectionIdxKind_Symbols], cv_data_from_symbol(arena, &symbols[i], CV_SymbolAlign)); }
  String8List raw_debug_s_list = cv_data_from_debug_s_c13(arena, &debug_s, 1);
  String8     raw_debug_s      = str8_list_join(arena, &raw_debug_s_list, 0);

  T_Ok(t_write_def_obj("debug.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "debug_s", ".debug$S", raw_debug_s, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
      {0}
    }
  }));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /debug:full /out:a.exe entry.obj debug.obj");
  T_Ok(g_last_exit_code == 0);

  String8        raw_pdb     = t_read_file(arena, str8_lit("a.pdb"));
  MSF_Parsed    *msf         = msf_parsed_from_data(arena, raw_pdb);
  String8        dbi_data    = msf_data_from_stream(msf, PDB_FixedStream_Dbi);
  PDB_DbiParsed *dbi         = pdb_dbi_from_data(arena, dbi_data);
  String8        gsi_data    = msf_data_from_stream(msf, dbi->gsi_sn);
  PDB_GsiParsed *gsi         = pdb_gsi_from_data(arena, gsi_data);
  String8        symbol_data = msf_data_from_stream(msf, dbi->sym_sn);

  for EachElement(i, symbols) {
    CV_Symbol symbol = symbols[i];

    String8 string     = cv_name_from_symbol(symbols[i].kind, symbols[i].data);
    U64     symbol_off = pdb_gsi_symbol_from_string(gsi, symbol_data, string);
    T_Ok(symbol_off < symbol_data.size);

    CV_Symbol test_symbol = {0};
    U64 test_symbol_size = cv_read_symbol(str8_skip(symbol_data, symbol_off), 0, 1, &test_symbol);
    T_Ok(test_symbol_size > 0);

    String8 test_symbol_name = cv_name_from_symbol(test_symbol.kind, test_symbol.data);
    T_Ok(str8_match(test_symbol_name, string, 0));
  }
}

TEST(validate_gsi_procs_and_typedefs)
{
  String8 raw_symbols[] = {
    cv_make_symbol(arena, CV_SymKind_OBJNAME, cv_make_obj_name(arena, str8_lit("debug.obj"), 0x123)),
    cv_make_symbol(arena, CV_SymKind_GPROC32_ID, cv_make_proc32(arena, (CV_SymProc32){0}, str8_lit("global_proc"))),
    cv_make_symbol(arena, CV_SymKind_PROC_ID_END, cv_make_end(arena)),

    cv_make_symbol(arena, CV_SymKind_UDT, cv_make_udt(arena, (CV_SymUDT){0}, str8_lit("global_typedef"))),

    cv_make_symbol(arena, CV_SymKind_OBJNAME, cv_make_obj_name(arena, str8_lit("debug.obj"), 0x123)),
    cv_make_symbol(arena, CV_SymKind_LPROC32_ID, cv_make_proc32(arena, (CV_SymProc32){0}, str8_lit("local_proc"))),
    cv_make_symbol(arena, CV_SymKind_UDT, cv_make_udt(arena, (CV_SymUDT){0}, str8_lit("local_typedef"))),
    cv_make_symbol(arena, CV_SymKind_PROC_ID_END, cv_make_end(arena)),
  };

  CV_Symbol symbols[ArrayCount(raw_symbols)] = {0};
  for EachElement(i, raw_symbols) { symbols[i] = cv_symbol_from_ptr(raw_symbols[i].str); }

  CV_DebugS debug_s = {0};
  for EachElement(i, symbols) { str8_list_push(arena, &debug_s.data_list[CV_C13SubSectionIdxKind_Symbols], cv_data_from_symbol(arena, &symbols[i], CV_SymbolAlign)); }
  String8List raw_debug_s_list = cv_data_from_debug_s_c13(arena, &debug_s, 1);
  String8     raw_debug_s      = str8_list_join(arena, &raw_debug_s_list, 0);

  T_Ok(t_write_def_obj("debug.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "debug_s", ".debug$S", raw_debug_s, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
      {0}
    }
  }));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /debug:full /out:a.exe entry.obj debug.obj");
  T_Ok(g_last_exit_code == 0);

  String8        raw_pdb     = t_read_file(arena, str8_lit("a.pdb"));
  MSF_Parsed    *msf         = msf_parsed_from_data(arena, raw_pdb);
  String8        dbi_data    = msf_data_from_stream(msf, PDB_FixedStream_Dbi);
  PDB_DbiParsed *dbi         = pdb_dbi_from_data(arena, dbi_data);
  String8        gsi_data    = msf_data_from_stream(msf, dbi->gsi_sn);
  PDB_GsiParsed *gsi         = pdb_gsi_from_data(arena, gsi_data);
  String8        symbol_data = msf_data_from_stream(msf, dbi->sym_sn);

  struct {
    char *name;
    B32   is_global;
    U64   offset;
    U64   imod;
  } procs[] = {
    { "global_proc", 1, 0x18, 2, },
    { "local_proc",  0, 0x64, 2, },
  };
  for EachElement(i, procs) {
    U64 symbol_off = pdb_gsi_symbol_from_string(gsi, symbol_data, str8_cstring(procs[i].name));
    T_Ok(symbol_off < symbol_data.size);

    CV_Symbol test_symbol      = {0};
    U64       test_symbol_size = cv_read_symbol(str8_skip(symbol_data, symbol_off), 0, 1, &test_symbol);
    String8   test_symbol_name = cv_name_from_symbol(test_symbol.kind, test_symbol.data);
    T_Ok(test_symbol_size > 0);
    T_Ok(str8_match(str8_cstring(procs[i].name), test_symbol_name, 0));
    T_Ok(test_symbol.kind == (procs[i].is_global ? CV_SymKind_PROCREF : CV_SymKind_LPROCREF));

    CV_SymRef2 *proc_ref = str8_deserial_get_raw_ptr(test_symbol.data, 0, sizeof(*proc_ref));
    T_Ok(proc_ref->suc_name == 0);
    T_Ok(proc_ref->sym_off == procs[i].offset);
    T_Ok(proc_ref->imod == procs[i].imod);
  }

  struct {
    char *name;
    B32   is_global;
  } typedefs[] = {
    { "global_typedef", 1 },
    { "local_typedef",  0 },
  };
  for EachElement(i, typedefs) {
    U64 symbol_off = pdb_gsi_symbol_from_string(gsi, symbol_data, str8_cstring(typedefs[i].name));
    if (typedefs[i].is_global) {
      T_Ok(symbol_off < symbol_data.size);
      CV_Symbol test_symbol      = {0};
      U64       test_symbol_size = cv_read_symbol(str8_skip(symbol_data, symbol_off), 0, 1, &test_symbol);
      String8   test_symbol_name = cv_name_from_symbol(test_symbol.kind, test_symbol.data);
      T_Ok(test_symbol_size > 0);
      T_Ok(str8_match(str8_cstring(typedefs[i].name), test_symbol_name, 0));
      T_Ok(test_symbol.kind == CV_SymKind_UDT);
    } else {
      T_Ok(symbol_off >= symbol_data.size);
    }
  }
}

TEST(validate_psi)
{
  T_Ok(t_write_def_obj("test.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "text", ".text", str8_lit("FOOBAR"), .flags = "rx:code" },
      { "data", ".data", str8_lit("QWE"), .flags = "rw:data" },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_ExternFunc("global_func", "text", 1),
      T_COFF_DefSymbol_Extern("global_var", "data", 1),
      T_COFF_DefSymbol_Static("static_var", "data", 1),
      {0}
    }
  }));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /debug:full entry.obj test.obj");
  T_Ok(g_last_exit_code == 0);

  String8        raw_pdb      = t_read_file(arena, str8_lit("a.pdb"));
  MSF_Parsed    *msf          = msf_parsed_from_data(arena, raw_pdb);
  String8        dbi_data     = msf_data_from_stream(msf, PDB_FixedStream_Dbi);
  PDB_DbiParsed *dbi          = pdb_dbi_from_data(arena, dbi_data);
  String8        psi_data     = msf_data_from_stream(msf, dbi->psi_sn);
  String8        psi_gsi_data = str8_range(psi_data.str + sizeof(PDB_PsiHeader), psi_data.str+psi_data.size);
  PDB_GsiParsed *psi          = pdb_gsi_from_data(arena, psi_gsi_data);
  String8        symbol_data  = msf_data_from_stream(msf, dbi->sym_sn);

  struct {
    char *name;
    B32 is_global;
    CV_Pub32Flags flags;
  }  pubs[] = {
    { "global_func", 1, CV_Pub32Flag_Function },
    { "global_var", 1, 0 },
    { "static_var", 0, 0 },
  };
  for EachElement(i, pubs) {
    U64 symbol_off = pdb_gsi_symbol_from_string(psi, symbol_data, str8_cstring(pubs[i].name));
    if (pubs[i].is_global) {
      T_Ok(symbol_off < symbol_data.size);
      CV_Symbol test_symbol      = {0};
      U64       test_symbol_size = cv_read_symbol(str8_skip(symbol_data, symbol_off), 0, 1, &test_symbol);
      String8   test_symbol_name = cv_name_from_symbol(test_symbol.kind, test_symbol.data);
      T_Ok(test_symbol_size > 0);
      T_Ok(str8_match(str8_cstring(pubs[i].name), test_symbol_name, 0));
      T_Ok(test_symbol.kind == CV_SymKind_PUB32);
      CV_SymPub32 *pub32 = str8_deserial_get_raw_ptr(test_symbol.data, 0, sizeof(*pub32));
      T_Ok(pub32->sec > 0);
      T_Ok(pubs[i].flags == pub32->flags);
    } else {
      T_Ok(symbol_off >= symbol_data.size);
    }
  }

  U64 pub32_count = 0;
  for EachElement(i, psi->buckets) {
    PDB_GsiBucket bucket = psi->buckets[i];
    for EachIndex(k, bucket.count) {
      CV_Symbol test_symbol      = {0};
      U64       test_symbol_size = cv_read_symbol(str8_skip(symbol_data, bucket.offs[k]), 0, 1, &test_symbol);
      T_Ok(test_symbol_size >= sizeof(CV_SymPub32));
      T_Ok(test_symbol.kind == CV_SymKind_PUB32);
      pub32_count += 1;
    }
  }
  T_Ok(pub32_count > 0);
}

TEST(pdbstripped)
{
  String8 debug_obj;
  {
    String8 raw_symbols[] = {
      cv_make_symbol(arena, CV_SymKind_OBJNAME, cv_make_obj_name(arena, str8_lit("debug.obj"), 0x123)),
      cv_make_symbol(arena, CV_SymKind_GPROC32_ID, cv_make_proc32(arena, (CV_SymProc32){0}, str8_lit("global_proc"))),
      cv_make_symbol(arena, CV_SymKind_PROC_ID_END, cv_make_end(arena)),

      cv_make_symbol(arena, CV_SymKind_UDT, cv_make_udt(arena, (CV_SymUDT){0}, str8_lit("global_typedef"))),

      cv_make_symbol(arena, CV_SymKind_LPROC32_ID, cv_make_proc32(arena, (CV_SymProc32){0}, str8_lit("local_proc"))),
      cv_make_symbol(arena, CV_SymKind_UDT, cv_make_udt(arena, (CV_SymUDT){0}, str8_lit("local_typedef"))),
      cv_make_symbol(arena, CV_SymKind_PROC_ID_END, cv_make_end(arena)),
    };

    CV_Symbol symbols[ArrayCount(raw_symbols)] = {0};
    for EachElement(i, raw_symbols) { symbols[i] = cv_symbol_from_ptr(raw_symbols[i].str); }

    CV_DebugS debug_s = {0};
    for EachElement(i, symbols) { str8_list_push(arena, &debug_s.data_list[CV_C13SubSectionIdxKind_Symbols], cv_data_from_symbol(arena, &symbols[i], CV_SymbolAlign)); }
    String8List raw_debug_s_list = cv_data_from_debug_s_c13(arena, &debug_s, 1);
    String8     raw_debug_s      = str8_list_join(arena, &raw_debug_s_list, 0);

    debug_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "debug_s", ".debug$S", raw_debug_s, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
        {0}
      }
    });
  }

  String8 pub_obj;
  {
    pub_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "text", ".text", str8_lit("FOOBAR"), .flags = "rx:code" },
        { "data", ".data", str8_lit("QWE"), .flags = "rw:data" },
        {0}
      },
      .symbols = (T_COFF_DefSymbol[]){
        T_COFF_DefSymbol_ExternFunc("global_func", "text", 1),
        T_COFF_DefSymbol_Extern("global_var", "data", 1),
        T_COFF_DefSymbol_Static("static_var", "data", 1),
        {0}
      }
    });
  }

  T_Ok(t_write_file(str8_lit("debug.obj"), debug_obj));
  T_Ok(t_write_file(str8_lit("pub.obj"), pub_obj));
  T_Ok(t_write_entry_obj());
  t_invoke_linkerf("/subsystem:console /entry:entry /debug:full /out:a.exe /pdbstripped:a.stripped.pdb entry.obj pub.obj debug.obj");
  T_Ok(g_last_exit_code == 0);

  String8        raw_pdb     = t_read_file(arena, str8_lit("a.stripped.pdb"));
  MSF_Parsed    *msf         = msf_parsed_from_data(arena, raw_pdb);
  String8        dbi_data    = msf_data_from_stream(msf, PDB_FixedStream_Dbi);
  PDB_DbiParsed *dbi         = pdb_dbi_from_data(arena, dbi_data);

  String8 mods_data = pdb_data_from_dbi_range(dbi, PDB_DbiRange_ModuleInfo);
  PDB_CompUnitArray *mods = pdb_comp_unit_array_from_data(arena, mods_data);
  T_Ok(mods->count > 0);

  // modules must contain only stubs for static procs
  for EachIndex(i, mods->count) {
    PDB_CompUnit *mod = mods->units[i];
    U64 sym_data_size = mod->range_off[PDB_DbiCompUnitRange_Symbols + 1] - mod->range_off[PDB_DbiCompUnitRange_Symbols];
    U64 c11_data_size = mod->range_off[PDB_DbiCompUnitRange_C11 + 1] - mod->range_off[PDB_DbiCompUnitRange_C11];
    U64 c13_data_size = mod->range_off[PDB_DbiCompUnitRange_C13 + 1] - mod->range_off[PDB_DbiCompUnitRange_C13];
    T_Ok(c11_data_size == 0);
    T_Ok(c13_data_size == 0);
    if (str8_match(str8_skip_last_slash(mod->obj_name), str8_lit("debug.obj"), 0)) {
      T_Ok(sym_data_size > 0);
    } else {
      T_Ok(sym_data_size == 0);
    }

    String8 mod_data = msf_data_from_stream(msf, mod->sn);
    String8 sym_data = str8_substr(mod_data, r1u64(mod->range_off[PDB_DbiCompUnitRange_Symbols], mod->range_off[PDB_DbiCompUnitRange_Symbols + 1]));
    for (U64 cursor = 0; cursor < sym_data_size; ) {
      CV_Symbol symbol = {0};
      U64 read_size = cv_read_symbol(sym_data, cursor, PDB_SYMBOL_ALIGN, &symbol);
      T_Ok(read_size > 0);
      cursor += read_size;
      T_Ok(symbol.kind == CV_SymKind_LPROC32 || symbol.kind == CV_SymKind_END);
    }
  }

  // global symbol stream must have public and references to the static stubs
  String8 symbol_data = msf_data_from_stream(msf, dbi->sym_sn);
  for (U64 cursor = 0; cursor < symbol_data.size; ) {
    CV_Symbol symbol = {0};
    U64 read_size = cv_read_symbol(symbol_data, cursor, PDB_SYMBOL_ALIGN, &symbol);
    T_Ok(read_size > 0);
    cursor += read_size;
    T_Ok(symbol.kind == CV_SymKind_PUB32 ||
         symbol.kind == CV_SymKind_LPROCREF);
  }

  // types must be stripped
  String8 tpi = msf_data_from_stream(msf, PDB_FixedStream_Tpi);
  T_Ok(tpi.size == sizeof(PDB_TpiHeader));
  String8 ipi = msf_data_from_stream(msf, PDB_FixedStream_Ipi);
  T_Ok(ipi.size == sizeof(PDB_TpiHeader));
}

TEST(ghash_check_corrupt)
{
  String8List t = {0}; str8_serial_begin(arena, &t);
    str8_serial_push_u32(arena, &t, CV_Signature_C13);
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_STRUCTURE, str8_struct(&(CV_LeafStruct){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_UNION, str8_struct(&(CV_LeafUnion){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_ENUM, str8_struct(&(CV_LeafEnum){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
  String8 debug_t = str8_serial_end(arena, &t);

  String8List h = {0}; str8_serial_begin(arena, &h);
  String8 debug_h = str8_serial_end(arena, &h);

  String8 debug_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "debug_t", ".debug$T", debug_t, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
      { "debug_h", ".debug$H", debug_h, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
      {0}
    }
  });

  T_Ok(t_write_file(str8_lit("debug.obj"), debug_obj));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /debug:ghash entry.obj debug.obj");
  T_Ok(g_last_exit_code == 0);

  B32 is_warning_found = 0;
  String8 debug_obj_path = t_make_file_path(arena, str8_lit("debug.obj"));
  String8 expected_line = str8f(arena, "Warning(%03u): %S: .debug$H section is too small to contain the header", LNK_Warning_GHash, debug_obj_path, LLVM_GHash_Magic);
  for (String8 i = g_errors; i.size > 0 && !is_warning_found; ) {
    String8 line = t_chop_line(&i);
    is_warning_found = str8_match(expected_line, line, StringMatchFlag_CaseInsensitive);
  }
  T_Ok(is_warning_found);
}

TEST(ghash_check_magic)
{
  String8List t = {0}; str8_serial_begin(arena, &t);
    str8_serial_push_u32(arena, &t, CV_Signature_C13);
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_STRUCTURE, str8_struct(&(CV_LeafStruct){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_UNION, str8_struct(&(CV_LeafUnion){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_ENUM, str8_struct(&(CV_LeafEnum){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
  String8 debug_t = str8_serial_end(arena, &t);

#if 0
  U64 *hashes = push_array(arena, U64, t.node_count);
  U64 i = 0;
  for EachNode(n, String8Node, t.first->next) {
    blake3(&hashes[i], sizeof(hashes[i]), n->string.str, n->string.size);
    i += 1;
  }
#endif
  String8List h = {0}; str8_serial_begin(arena, &h);
    str8_serial_push_struct(arena, &h, (&(LLVM_GHash){ .magic = 123, .hash_alg = LLVM_GHashAlg_BLAKE3, .version = LLVM_GHash_CurrentVersion }));
    str8_serial_push_u64(arena, &h, 0x6ebacae08af4fda5ull);
    str8_serial_push_u64(arena, &h, 0xc385876694f9769aull);
    str8_serial_push_u64(arena, &h, 0x7ea8a529a89b2288ull);
  String8 debug_h = str8_serial_end(arena, &h);

  String8 debug_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "debug_t", ".debug$T", debug_t, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
      { "debug_h", ".debug$H", debug_h, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
      {0}
    }
  });

  T_Ok(t_write_file(str8_lit("debug.obj"), debug_obj));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /debug:ghash entry.obj debug.obj");
  T_Ok(g_last_exit_code == 0);

  B32 is_warning_found = 0;
  String8 debug_obj_path = t_make_file_path(arena, str8_lit("debug.obj"));
  String8 expected_line = str8f(arena, "Warning(%03u): %S: .debug$H contains invalid magic: got 0x7b, expected 0x%x", LNK_Warning_GHash, debug_obj_path, LLVM_GHash_Magic);
  for (String8 i = g_errors; i.size > 0 && !is_warning_found; ) {
    String8 line = t_chop_line(&i);
    is_warning_found = str8_match(expected_line, line, StringMatchFlag_CaseInsensitive);
  }
  T_Ok(is_warning_found);
}

TEST(ghash_check_version)
{
  String8List t = {0}; str8_serial_begin(arena, &t);
    str8_serial_push_u32(arena, &t, CV_Signature_C13);
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_STRUCTURE, str8_struct(&(CV_LeafStruct){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_UNION, str8_struct(&(CV_LeafUnion){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_ENUM, str8_struct(&(CV_LeafEnum){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
  String8 debug_t = str8_serial_end(arena, &t);

#if 0
  U64 *hashes = push_array(arena, U64, t.node_count);
  U64 i = 0;
  for EachNode(n, String8Node, t.first->next) {
    blake3(&hashes[i], sizeof(hashes[i]), n->string.str, n->string.size);
    i += 1;
  }
#endif
  String8List h = {0}; str8_serial_begin(arena, &h);
    str8_serial_push_struct(arena, &h, (&(LLVM_GHash){ .magic = LLVM_GHash_Magic, .hash_alg = LLVM_GHashAlg_BLAKE3, .version = 0xbeef }));
    str8_serial_push_u64(arena, &h, 0x6ebacae08af4fda5ull);
    str8_serial_push_u64(arena, &h, 0xc385876694f9769aull);
    str8_serial_push_u64(arena, &h, 0x7ea8a529a89b2288ull);
  String8 debug_h = str8_serial_end(arena, &h);

  String8 debug_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "debug_t", ".debug$T", debug_t, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
      { "debug_h", ".debug$H", debug_h, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
      {0}
    }
  });

  T_Ok(t_write_file(str8_lit("debug.obj"), debug_obj));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /debug:ghash entry.obj debug.obj");
  T_Ok(g_last_exit_code == 0);

  B32 is_warning_found = 0;
  String8 debug_obj_path = t_make_file_path(arena, str8_lit("debug.obj"));
  String8 expected_line = str8f(arena, "Warning(%03u): %S: mismatched .debug$H version: got %u, expected %u", LNK_Warning_GHash, debug_obj_path, 0xbeef, LLVM_GHash_CurrentVersion);
  for (String8 i = g_errors; i.size > 0 && !is_warning_found; ) {
    String8 line = t_chop_line(&i);
    is_warning_found = str8_match(expected_line, line, StringMatchFlag_CaseInsensitive);
  }
  T_Ok(is_warning_found);
}

TEST(ghash_check_hash_alg)
{
  String8List t = {0}; str8_serial_begin(arena, &t);
    str8_serial_push_u32(arena, &t, CV_Signature_C13);
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_STRUCTURE, str8_struct(&(CV_LeafStruct){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_UNION, str8_struct(&(CV_LeafUnion){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_ENUM, str8_struct(&(CV_LeafEnum){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
  String8 debug_t = str8_serial_end(arena, &t);

  String8List h = {0}; str8_serial_begin(arena, &h);
    str8_serial_push_struct(arena, &h, (&(LLVM_GHash){ .magic = LLVM_GHash_Magic, .hash_alg = LLVM_GHashAlg_SHA1_8, .version = LLVM_GHash_CurrentVersion }));
    str8_serial_push_u64(arena, &h, 0x6ebacae08af4fda5ull);
    str8_serial_push_u64(arena, &h, 0xc385876694f9769aull);
    str8_serial_push_u64(arena, &h, 0x7ea8a529a89b2288ull);
  String8 debug_h = str8_serial_end(arena, &h);

  String8 debug_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "debug_t", ".debug$T", debug_t, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
      { "debug_h", ".debug$H", debug_h, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
      {0}
    }
  });

  T_Ok(t_write_file(str8_lit("debug.obj"), debug_obj));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /debug:ghash entry.obj debug.obj");
  T_Ok(g_last_exit_code == 0);

  B32     is_warning_found = 0;
  String8 debug_obj_path   = t_make_file_path(arena, str8_lit("debug.obj"));
  String8 expected_line    = str8f(arena, "Warning(%03u): %S: mismatched .debug$H hash algorithm: got SHA1_8, expected BALK3", LNK_Warning_GHash, debug_obj_path);
  for (String8 i = g_errors; i.size > 0 && !is_warning_found; ) {
    String8 line = t_chop_line(&i);
    is_warning_found = str8_match(expected_line, line, StringMatchFlag_CaseInsensitive);
  }
  T_Ok(is_warning_found);
}

TEST(ghash_match_debug_t)
{
  String8List t = {0}; str8_serial_begin(arena, &t);
    str8_serial_push_u32(arena, &t, CV_Signature_C13);
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_STRUCTURE, str8_struct(&(CV_LeafStruct){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
    str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_UNION, str8_struct(&(CV_LeafUnion){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
    //str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_ENUM, str8_struct(&(CV_LeafEnum){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
  String8 debug_t = str8_serial_end(arena, &t);

  String8List h = {0}; str8_serial_begin(arena, &h);
    str8_serial_push_struct(arena, &h, (&(LLVM_GHash){ .magic = LLVM_GHash_Magic, .hash_alg = LLVM_GHashAlg_BLAKE3, .version = LLVM_GHash_CurrentVersion }));
    str8_serial_push_u64(arena, &h, 0x6ebacae08af4fda5ull);
    str8_serial_push_u64(arena, &h, 0xc385876694f9769aull);
    str8_serial_push_u64(arena, &h, 0x7ea8a529a89b2288ull);
  String8 debug_h = str8_serial_end(arena, &h);

  String8 debug_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "debug_t", ".debug$T", debug_t, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
      { "debug_h", ".debug$H", debug_h, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
      {0}
    }
  });

  T_Ok(t_write_file(str8_lit("debug.obj"), debug_obj));
  T_Ok(t_write_entry_obj());

  String8 output = {0};
  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /debug:ghash entry.obj debug.obj");
  T_Ok(g_last_exit_code == 0);

  B32     is_warning_found = 0;
  String8 debug_obj_path   = t_make_file_path(arena, str8_lit("debug.obj"));
  String8 expected_line    = str8f(arena, "Warning(%03u): %S: mismatched .debug$H hash count and type count: got 3 hashes for 2 types", LNK_Warning_GHash, debug_obj_path);
  for (String8 i = g_errors; i.size > 0 && !is_warning_found; ) {
    String8 line = t_chop_line(&i);
    is_warning_found = str8_match(expected_line, line, StringMatchFlag_CaseInsensitive);
  }
  T_Ok(is_warning_found);
}

TEST(ghash_basic)
{
  String8 a_obj;
  {
    String8List t = {0}; str8_serial_begin(arena, &t);
      str8_serial_push_u32(arena, &t, CV_Signature_C13);
      str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_STRUCTURE, str8_struct(&(CV_LeafStruct){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
      str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_UNION, str8_struct(&(CV_LeafUnion){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
      str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_ENUM, str8_struct(&(CV_LeafEnum){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
    String8 debug_t = str8_serial_end(arena, &t);

    String8List h = {0}; str8_serial_begin(arena, &h);
      str8_serial_push_struct(arena, &h, (&(LLVM_GHash){ .magic = LLVM_GHash_Magic, .hash_alg = LLVM_GHashAlg_BLAKE3, .version = LLVM_GHash_CurrentVersion }));
      str8_serial_push_u64(arena, &h, 1);
      str8_serial_push_u64(arena, &h, 2);
      str8_serial_push_u64(arena, &h, 3);
    String8 debug_h = str8_serial_end(arena, &h);

    a_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "debug_t", ".debug$T", debug_t, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
        { "debug_h", ".debug$H", debug_h, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
        {0}
      }
    });
  }

  String8 b_obj;
  {
    String8List t = {0}; str8_serial_begin(arena, &t);
      str8_serial_push_u32(arena, &t, CV_Signature_C13);
      str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_STRUCTURE, str8_struct(&(CV_LeafStruct){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
      str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_UNION, str8_struct(&(CV_LeafUnion){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
      str8_serial_push_string(arena, &t, cv_make_leaf(arena, CV_LeafKind_ENUM, str8_struct(&(CV_LeafEnum){ .props = CV_TypeProp_FwdRef }), CV_LeafAlign));
    String8 debug_t = str8_serial_end(arena, &t);

    String8List h = {0}; str8_serial_begin(arena, &h);
      str8_serial_push_struct(arena, &h, (&(LLVM_GHash){ .magic = LLVM_GHash_Magic, .hash_alg = LLVM_GHashAlg_BLAKE3, .version = LLVM_GHash_CurrentVersion }));
      str8_serial_push_u64(arena, &h, 4);
      str8_serial_push_u64(arena, &h, 5);
      str8_serial_push_u64(arena, &h, 6);
    String8 debug_h = str8_serial_end(arena, &h);

    b_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
      .machine = T_COFF_DefSetMachine(X64),
      .sections = (T_COFF_DefSection[]){
        { "debug_t", ".debug$T", debug_t, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
        { "debug_h", ".debug$H", debug_h, .flags = "r:data", .raw_flags = COFF_SectionFlag_MemDiscardable },
        {0}
      }
    });
  }

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("b.obj"), b_obj));
  T_Ok(t_write_entry_obj());

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /debug:ghash entry.obj a.obj b.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8        raw_pdb    = t_read_file(arena, str8_lit("a.pdb"));
    MSF_Parsed    *msf_parsed = msf_parsed_from_data(arena, raw_pdb);
    String8        raw_tpi    = msf_data_from_stream(msf_parsed, PDB_FixedStream_Tpi);
    PDB_TpiParsed *tpi        = pdb_tpi_from_data(arena, raw_tpi);
    U64 leaf_count = tpi->itype_opl - tpi->itype_first;
    T_Ok(leaf_count == 6);
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /debug:full entry.obj a.obj b.obj");
  T_Ok(g_last_exit_code == 0);

  {
    String8        raw_pdb    = t_read_file(arena, str8_lit("a.pdb"));
    MSF_Parsed    *msf_parsed = msf_parsed_from_data(arena, raw_pdb);
    String8        raw_tpi    = msf_data_from_stream(msf_parsed, PDB_FixedStream_Tpi);
    PDB_TpiParsed *tpi        = pdb_tpi_from_data(arena, raw_tpi);
    U64 leaf_count = tpi->itype_opl - tpi->itype_first;
    T_Ok(leaf_count == 3);
  }
}

TEST(patch_cv_symbol_tree)
{
  String8List raw_symbols = {0};
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_OBJNAME,        cv_make_obj_name(arena, str8_lit("foo.obj"), 123)));
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_GPROC32,        cv_make_proc32(arena, (CV_SymProc32){0}, str8_lit("Proc"))));
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_INLINESITE,     cv_make_inline_site(arena, (CV_SymInlineSite){0}, str8_zero())));
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_INLINESITE_END, cv_make_inline_site_end(arena)));
  str8_list_push(arena, &raw_symbols, cv_make_symbol(arena, CV_SymKind_END,            cv_make_end(arena)));

  U64 tree_size = cv_patch_symbol_tree_offsets(raw_symbols, sizeof(CV_Signature), 4);
  T_Ok(tree_size == 84);

  {
    String8Node buf     = *raw_symbols.first;
    U64         buf_pos = 0;

    CV_SymbolHeader obj_header;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(obj_header), &obj_header) == sizeof(obj_header));
    T_Ok(obj_header.kind == CV_SymKind_OBJNAME);
    T_Ok(str8_buffer_skip(&buf, &buf_pos, obj_header.size - sizeof(CV_SymKind)));

    CV_SymbolHeader proc_header;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(proc_header), &proc_header) == sizeof(proc_header));
    T_Ok(proc_header.kind == CV_SymKind_GPROC32);

    CV_SymProc32 proc;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(proc), &proc) == sizeof(proc));
    T_Ok(proc.end == 0x54);
    T_Ok(str8_buffer_skip(&buf, &buf_pos, proc_header.size - sizeof(CV_SymKind) - sizeof(proc)));

    CV_SymbolHeader inline_site_header;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(inline_site_header), &inline_site_header) == sizeof(inline_site_header));
    T_Ok(inline_site_header.kind == CV_SymKind_INLINESITE);

    CV_SymInlineSite inline_site;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(inline_site), &inline_site));
    T_Ok(inline_site.parent == 0x14);
    T_Ok(inline_site.end == 0x50);
    T_Ok(str8_buffer_skip(&buf, &buf_pos, inline_site_header.size - sizeof(CV_SymKind) - sizeof(inline_site)));

    CV_SymbolHeader inline_end_header;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(inline_end_header), &inline_end_header) == sizeof(inline_end_header));
    T_Ok(inline_end_header.kind == CV_SymKind_INLINESITE_END);

    CV_SymbolHeader proc_end_header;
    T_Ok(str8_buffer_read(&buf, &buf_pos, sizeof(proc_end_header), &proc_end_header) == sizeof(proc_end_header));
    T_Ok(proc_end_header.kind == CV_SymKind_END);

    T_Ok(buf.string.size == 0);
    T_Ok(buf.string.str == 0);
    T_Ok(buf_pos == 0);
  }
}

TEST(whole_archive)
{
  T_Ok(t_write_entry_obj());
  T_Ok(t_write_def_lib("a.lib", (T_COFF_DefLib){
    .emit_second_member = 1,
    .members = (T_COFF_DefLibMember[]){
      {
        .type = T_COFF_DefLibMember_Obj,
        .obj = {
          .path = str8_lit("a.obj"),
          .machine = T_COFF_DefSetMachine(X64),
          .sections = (T_COFF_DefSection[]){
            { "a", ".a", str8_lit("a"), .flags = "rw:data" },
            {0}
          }
        }
      },
      {0}
    }
  }));
  T_Ok(t_write_def_lib("b.lib", (T_COFF_DefLib){
    .emit_second_member = 1,
    .members = (T_COFF_DefLibMember[]){
      {
        .type = T_COFF_DefLibMember_Obj,
        .obj = {
          .path = str8_lit("b.obj"),
          .machine = T_COFF_DefSetMachine(X64),
          .sections = (T_COFF_DefSection[]){
            { "b", ".b", str8_lit("b"), .flags = "rw:data" },
            {0}
          }
        }
      },
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:all_libs.exe entry.obj /wholearchive a.lib b.lib");
  T_Ok(g_last_exit_code == 0);
  {
    String8              exe           = t_read_file(arena, str8_lit("all_libs.exe"));
    PE_BinInfo           pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8              string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader  *a_sect        = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a"));
    COFF_SectionHeader  *b_sect        = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".b"));
    T_Ok(a_sect != 0);
    T_Ok(b_sect != 0);
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:only_a.exe entry.obj /wholearchive:a.lib a.lib b.lib");
  T_Ok(g_last_exit_code == 0);
  {
    String8              exe           = t_read_file(arena, str8_lit("only_a.exe"));
    PE_BinInfo           pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8              string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader  *a_sect        = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a"));
    COFF_SectionHeader  *b_sect        = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".b"));
    T_Ok(a_sect != 0);
    T_Ok(b_sect == 0);
  }

  t_invoke_linkerf("/subsystem:console /entry:entry /out:only_b.exe /wholearchive:b.lib a.lib b.lib entry.obj");
  T_Ok(g_last_exit_code == 0);
  {
    String8              exe           = t_read_file(arena, str8_lit("only_b.exe"));
    PE_BinInfo           pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8              string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader  *a_sect        = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".a"));
    COFF_SectionHeader  *b_sect        = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".b"));
    T_Ok(a_sect == 0);
    T_Ok(b_sect != 0);
  }
}
#if OS_WINDOWS

internal B32
t_radlink_validate_asan_out(String8 obj_name)
{
  Temp scratch = scratch_begin(0,0);
  B32 is_ok = 0;

  t_invoke_linkerf("%S /debug:full", obj_name);
  if (g_last_exit_code != 0) { goto exit; }

  String8 exe_path = t_make_file_path(scratch.arena, str8f(scratch.arena, "%S.exe", str8_chop_last_dot(obj_name)));

  char *old_path_cstr = getenv("PATH");
  String8List env = {0};
  str8_list_pushf(scratch.arena, &env, "PATH=%S;%S", str8_chop_last_slash(t_cl_path()), str8_cstring(old_path_cstr));
  t_invoke_env(exe_path, str8_zero(), env, max_U64);

  String8 s = g_errors;

  String8 header = t_chop_line(&s);
  if ( ! str8_match(header, str8_lit("================================================================="), 0)) {
    goto exit;
  }

  String8 cause = t_chop_line(&s);
  if ( str8_find_needle(cause, 0, str8_lit("AddressSanitizer: heap-use-after-free on address"), 0) >= cause.size) {
    goto exit;
  }

  is_ok = 1;
  exit:;
  scratch_end(scratch);
  return is_ok;
}

TEST(infer_asan)
{
  char *program = 
    "#include <stdlib.h>\n"
    " int main(void) {\n"
    "int *foo = malloc(sizeof(*foo));\n"
    "free(foo);\n"
    "*foo = 1;\n"
    "}\n"
    ;

  // /MD
  {
    T_Ok(t_write_file(str8_lit("main.c"), str8_cstring(program)));
    t_invoke_cl("/MD /fsanitize=address /Z7 /c /Fo:main_md.obj main.c");
    T_Ok(g_last_exit_code == 0);
    T_Ok(t_radlink_validate_asan_out(str8_lit("main_md.obj")));
  }

  // /MDd
  {
    T_Ok(t_write_file(str8_lit("main.c"), str8_cstring(program)));
    t_invoke_cl("/MDd /fsanitize=address /Z7 /c /Fo:main_mdd.obj main.c");
    T_Ok(g_last_exit_code == 0);
    T_Ok(t_radlink_validate_asan_out(str8_lit("main_mdd.obj")));
  }

  // /MT
  {
    T_Ok(t_write_file(str8_lit("main.c"), str8_cstring(program)));
    t_invoke_cl("/MT /fsanitize=address /Z7 /c /Fo:main_mt.obj main.c");
    T_Ok(g_last_exit_code == 0);
    T_Ok(t_radlink_validate_asan_out(str8_lit("main_mt.obj")));
  }

  // /MTd
  {
    T_Ok(t_write_file(str8_lit("main.c"), str8_cstring(program)));
    t_invoke_cl("/MT /fsanitize=address /Z7 /c /Fo:main_mtd.obj main.c");
    T_Ok(g_last_exit_code == 0);
    T_Ok(t_radlink_validate_asan_out(str8_lit("main_mtd.obj")));
  }
}

#endif

#if OS_WINDOWS
TEST(determ_test)
{
  // compile the test target (torture)
  t_invoke_cl("/fsanitize=address /c /Z7 /Fo:test.obj -I%S /Zc:preprocessor %S/torture/torture_main.c", t_src_path(), t_src_path());
  T_Ok(g_last_exit_code == 0);

  U64 run_count = 25;
  T_Ok(run_count > 1);
  String8 test_path = t_make_file_path(arena, str8_lit("test.obj"));

  // single-threaded link
  t_invoke_linkerf("%S /debug:full /rad_time_stamp:0 /rad_workers:1 /pdbaltpath:main.pdb /rad_log:-all /rad_ignore:74 /out:main.exe", test_path);
  T_Ok(g_last_exit_code == 0);

  // read b
  String8 main_exe = t_read_file(arena, str8_lit("main.exe"));
  String8 main_pdb = t_read_file(arena, str8_lit("main.pdb"));

  // multi-threaded links
  OS_HandleList linkers = {0};
  for EachIndex(i, run_count) {
    String8 out_path = t_make_file_path(arena, str8f(arena, "%llu.exe", i));
    String8 cmdl = str8f(arena, "%S %S /debug:full /rad_time_stamp:0 /rad_imagealtpath:main.exe /pdbaltpath:main.pdb /rad_log:-all /rad_ignore:74 /out:%S", t_radlink_path(), test_path, out_path);
    OS_Handle process_handle = os_cmd_line_launch(cmdl);
    T_Ok(!os_handle_match(os_handle_zero(), process_handle));
    os_handle_list_push(arena, &linkers, process_handle);
  }

  // wait for linkers
  for EachNode(n, OS_HandleNode, linkers.first) { os_process_join(n->v, max_U64, 0); }

  for EachIndex(i, run_count) {
    Temp temp = temp_begin(arena);
    String8 exe = t_read_file(temp.arena, str8f(temp.arena, "%llu.exe", i));
    String8 pdb = t_read_file(temp.arena, str8f(temp.arena, "%llu.pdb", i));
    T_Ok(exe.size);
    T_Ok(pdb.size);
    T_Ok(str8_match(main_exe, exe, 0));
    T_Ok(str8_match(main_pdb, pdb, 0));
    temp_end(temp);
  }
}

#endif

#if 0

TEST(fold_two_funcs)
{
  U8 same_text[] = {
    0x48, 0x31, 0xc0, // xor rax, rax
    0xc3              // ret
  };
  U8 entry_text[] = {
    0xe8, 0x00, 0x00, 0x00, 0x00, // call a
    0xe8, 0x00, 0x00, 0x00, 0x00, // call b
    0xc3,                         // ret
  };
  T_Ok(t_write_def_obj("ident_funcs.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      { "a", ".text$mn", str8_array_fixed(same_text), .flags = "rx:code", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      { "b", ".text$mb", str8_array_fixed(same_text), .flags = "rx:code", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {
        "entry", ".text", str8_array_fixed(entry_text), .flags = "rx:code@1",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 1, "a"),
          T_COFF_DefReloc(X64_Rel32, 6, "b"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("a", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Secdef("b", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_ExternFunc("a", "a", 0),
      T_COFF_DefSymbol_ExternFunc("b", "b", 0),
      T_COFF_DefSymbol_ExternFunc("entry", "entry", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /opt:icf ident_funcs.obj");
  T_Ok(g_last_exit_code == 0);

  String8 exe = t_read_file(arena, str8_lit("ident_funcs.exe"));
  T_Ok(exe.size);

  PE_BinInfo           pe            = pe_bin_info_from_data(arena, exe);
  COFF_SectionHeader  *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
  String8              string_table  = str8_substr(exe, pe.string_table_range);
  COFF_SectionHeader  *text_sect     = coff_section_header_from_name(exe, section_table, pe.section_count, str8_lit(".text"));

  // validate .text header
  T_Ok(text_sect->voff  == 0x1000);
  T_Ok(text_sect->vsize >= 0x14);
  T_Ok(text_sect->fsize == 0x200);

  T_Ok(text_sect->foff + text_sect->vsize <= exe.size);
  String8 text_data = str8_substr(exe, r1u64(text_sect->foff, text_sect->foff + 0x14));

  U8 expected_text[] = {
    // entry
    0xe8, 0x0b, 0x00, 0x00, 0x00,
    0xe8, 0x06, 0x00, 0x00, 0x00,
    0xc3,

    // pad
    0xcc, 0xcc, 0xcc, 0xcc, 0xcc,

    // a and b folded
    0x48, 0x31, 0xc0,
    0xc3,
  };
  T_Ok(str8_match(text_data, str8_array_fixed(expected_text), 0));
}


TEST(same_but_different)
{
  U8 text[] = {
    0xe8, 0x00, 0x00, 0x00, 0x00, // call $
    0xc3
  };
  U8 return_1[] = {
    0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00, // mov rax, 1
    0xc3                                      // ret
  };
  U8 return_2[] = {
    0x48, 0xc7, 0xc0, 0x02, 0x00, 0x00, 0x00, // mov rax, 2
    0xc3                                      // ret
  };
  U8 call_a_and_b[] = {
    0xe8, 0x00, 0x00, 0x00, 0x00,
    0xe8, 0x00, 0x00, 0x00, 0x00,
    0xc3
  };
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "entry", ".text", str8_array_fixed(call_a_and_b), .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 1, "a"),
          T_COFF_DefReloc(X64_Rel32, 6, "b"),
          {0}
        }
      },
      {
        "a", ".text", str8_array_fixed(text), .flags = "rx:code", .raw_flags = COFF_SectionFlag_LnkCOMDAT,
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 1, "c"),
          {0}
        }
      },
      {
        "b", ".text", str8_array_fixed(text), .flags = "rx:code", .raw_flags = COFF_SectionFlag_LnkCOMDAT,
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 1, "d"),
          {0}
        }
      },
      { "c", ".text", str8_array_fixed(return_1), .flags = "rx:code", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      { "d", ".text", str8_array_fixed(return_2), .flags = "rx:code", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("a", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Secdef("b", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Secdef("c", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Secdef("d", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Extern("entry", "entry", 0),
      T_COFF_DefSymbol_Extern("a", "a", 0),
      T_COFF_DefSymbol_Extern("b", "b", 0),
      T_COFF_DefSymbol_Extern("c", "c", 0),
      T_COFF_DefSymbol_Extern("d", "d", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe /opt:icf a.obj");
  T_Ok(g_last_exit_code == 0);

  // validate output
  {
    U8 expected_text[] = {
      0xe8, 0x0b, 0x00, 0x00, 0x00, // call a
      0xe8, 0x16, 0x00, 0x00, 0x00, // call b
      0xc3,                        
      0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 
      0xe8, 0x1b, 0x00, 0x00, 0x00, // call c
      0xc3,                         
      0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
      0xe8, 0x1b, 0x00, 0x00, 0x00, // call d
      0xc3,
      0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
      0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00, // mov rax, 1
      0xc3,
      0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
      0x48, 0xc7, 0xc0, 0x02, 0x00, 0x00, 0x00, // mov rax, 2
      0xc3,
    };

    String8 exe = t_read_file(arena, str8_lit("a.exe"));
    T_Ok(exe.size);

    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));

    T_Ok(text_section);
    T_Ok(text_section->foff + sizeof(expected_text) <= exe.size);

    String8 text = str8_substr(exe, r1u64(text_section->foff, text_section->foff + text_section->vsize));
    T_Ok(str8_match(text, str8_array_fixed(expected_text), 0));
  }
}


TEST(fold_diamond)
{
  U8 call_b_and_c[] = {
    0xe8, 0x00, 0x00, 0x00, 0x00,
    0xe8, 0x00, 0x00, 0x00, 0x00,
    0xc3 
  };
  U8 call_and_return[] = {
    0xe8, 0x00, 0x00, 0x00, 0x00,
    0xc3 
  };
  U8 clear_and_return[] = {
    0x48, 0x31, 0xc0,
    0xc3
  };
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "a", ".text", str8_array_fixed(call_b_and_c), .flags = "rx:code", .raw_flags = COFF_SectionFlag_LnkCOMDAT,
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 1, "b"),
          T_COFF_DefReloc(X64_Rel32, 6, "c"),
          {0}
        }
      },
      {
        "b", ".text", str8_array_fixed(call_and_return), .flags = "rx:code", .raw_flags = COFF_SectionFlag_LnkCOMDAT,
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 1, "d"),
          {0}
        }
      },
      {
        "c", ".text", str8_array_fixed(call_and_return), .flags = "rx:code", .raw_flags = COFF_SectionFlag_LnkCOMDAT,
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 1, "d"),
          {0}
        }
      },
      { "d", ".text", str8_array_fixed(clear_and_return), .flags = "rx:code", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("a", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Secdef("b", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Secdef("c", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Secdef("d", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Extern("a", "a", 0),
      T_COFF_DefSymbol_Extern("b", "b", 0),
      T_COFF_DefSymbol_Extern("c", "c", 0),
      T_COFF_DefSymbol_Extern("d", "d", 0),
      {0}
    }
  }));
  t_invoke_linkerf("/subsystem:console /entry:a /out:a.exe /opt:icf a.obj");
  T_Ok(g_last_exit_code == 0);

  // validate output
  {
    U8 expected_text[] = {
      0xe8, 0x0b, 0x00, 0x00, 0x00,
      0xe8, 0x06, 0x00, 0x00, 0x00,
      0xc3,

      0xcc, 0xcc, 0xcc, 0xcc, 0xcc,

      0xe8, 0x0b, 0x00, 0x00, 0x00,
      0xc3,

      0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,

      0x48, 0x31, 0xc0,
      0xc3
    };

    String8 exe = t_read_file(arena, str8_lit("a.exe"));
    T_Ok(exe.size);

    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));

    T_Ok(text_section);
    T_Ok(text_section->foff + sizeof(expected_text) <= exe.size);

    String8 text = str8_substr(exe, r1u64(text_section->foff, text_section->foff + text_section->vsize));
    T_Ok(str8_match(text, str8_array_fixed(expected_text), 0));
  }
}


TEST(cyclic_icf)
{
  U8 text[] = {
    0xe8, 0x00, 0x00, 0x00, 0x00,
    0xc3
  };
  T_Ok(t_write_def_obj("a.obj", (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "a", ".text", str8_array_fixed(text), .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 1, "b"),
          {0}
        }
      },
      {
        "b", ".text", str8_array_fixed(text), .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 1, "a"),
          {0}
        }
      },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Extern("a", "a", 0),
      T_COFF_DefSymbol_Static("b", "b", 0),
      {0}
    }
  }));

  t_invoke_linkerf("/subsystem:console /out:a.exe /entry:a /opt:icf a.obj");
  T_Ok(g_last_exit_code == 0);

  // validate output
  {
    U8 expected_text[] = {
      0xe8, 0x0b, 0x00, 0x00, 0x00, // a
      0xc3,
      0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
      0xe8, 0xeb, 0xff, 0xff, 0xff, // b
      0xc3,            
    };

    String8 exe = t_read_file(arena, str8_lit("a.exe"));
    T_Ok(exe.size);

    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));

    T_Ok(text_section);
    T_Ok(text_section->foff + sizeof(expected_text) <= exe.size);

    String8 text = str8_substr(exe, r1u64(text_section->foff, text_section->foff + text_section->vsize));
    T_Ok(str8_match(text, str8_array_fixed(expected_text), 0));
  }
}


TEST(fold_with_largest_align)
{
  U8 text[] = {
    0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00, // mov rax, 1
    0xc3
  };
  U8 call_a_and_b[] = {
    0xe8, 0x00, 0x00, 0x00, 0x00,
    0xe8, 0x00, 0x00, 0x00, 0x00,
    0xc3 
  };

  String8 a_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "entry", ".text", str8_array_fixed(call_a_and_b), .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 1, "a"),
          T_COFF_DefReloc(X64_Rel32, 6, "b"),
          {0}
        }
      },
      { "a", ".text", str8_array_fixed(text), .flags = "rx:code@4", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      { "b", ".text", str8_array_fixed(text), .flags = "rx:code@8", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("a", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Secdef("b", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Extern("entry", "entry", 0),
      T_COFF_DefSymbol_Static("a", "a", 0),
      T_COFF_DefSymbol_Static("b", "b", 0),
      {0}
    }
  });

  // swap sections for a and b
  String8 b_obj = t_coff_from_def_obj(arena, (T_COFF_DefObj){
    .machine = T_COFF_DefSetMachine(X64),
    .sections = (T_COFF_DefSection[]){
      {
        "entry", ".text", str8_array_fixed(call_a_and_b), .flags = "rx:code",
        .relocs = (T_COFF_DefReloc[]){
          T_COFF_DefReloc(X64_Rel32, 1, "a"),
          T_COFF_DefReloc(X64_Rel32, 6, "b"),
          {0}
        }
      },
      { "a", ".text", str8_array_fixed(text), .flags = "rx:code@8", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      { "b", ".text", str8_array_fixed(text), .flags = "rx:code@4", .raw_flags = COFF_SectionFlag_LnkCOMDAT },
      {0}
    },
    .symbols = (T_COFF_DefSymbol[]){
      T_COFF_DefSymbol_Secdef("a", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Secdef("b", COFF_ComdatSelect_NoDuplicates),
      T_COFF_DefSymbol_Extern("entry", "entry", 0),
      T_COFF_DefSymbol_Static("a", "a", 0),
      T_COFF_DefSymbol_Static("b", "b", 0),
      {0}
    }
  });

  T_Ok(t_write_file(str8_lit("a.obj"), a_obj));
  T_Ok(t_write_file(str8_lit("b.obj"), b_obj));

  t_invoke_linkerf("/subsystem:console /entry:entry /out:a.exe a.obj");
  T_Ok(g_last_exit_code == 0);

  t_invoke_linkerf("/subsystem:console /entry:entry /out:b.exe b.obj");
  T_Ok(g_last_exit_code == 0);

  U8 expected_text[] = {
    0xe8, 0x0b, 0x00, 0x00, 0x00,   
    0xe8, 0x06, 0x00, 0x00, 0x00,   
    0xc3,               
    0xcc,
    0xcc,
    0xcc,
    0xcc,
    0xcc,
    0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00,
    0x00,
    0xc3,
  };

  // validate output in a.exe
  {
    String8 exe = t_read_file(arena, str8_lit("a.exe"));
    T_Ok(exe.size);

    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));

    T_Ok(text_section);
    T_Ok(text_section->foff + sizeof(expected_text) <= exe.size);

    String8 text = str8_substr(exe, r1u64(text_section->foff, text_section->foff + text_section->vsize));
    T_Ok(str8_match(text, str8_array_fixed(expected_text), 0));
  }

  // validate output in b.exe
  {
    String8 exe = t_read_file(arena, str8_lit("b.exe"));
    T_Ok(exe.size);

    PE_BinInfo          pe            = pe_bin_info_from_data(arena, exe);
    COFF_SectionHeader *section_table = (COFF_SectionHeader *)str8_substr(exe, pe.section_table_range).str;
    String8             string_table  = str8_substr(exe, pe.string_table_range);
    COFF_SectionHeader *text_section = coff_section_header_from_name(string_table, section_table, pe.section_count, str8_lit(".text"));

    T_Ok(text_section);
    T_Ok(text_section->foff + sizeof(expected_text) <= exe.size);

    String8 text = str8_substr(exe, r1u64(text_section->foff, text_section->foff + text_section->vsize));
    T_Ok(str8_match(text, str8_array_fixed(expected_text), 0));
  }
}

#endif

#undef T_Group
