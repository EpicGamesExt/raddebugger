// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal OBJ *
obj_alloc(U64 time_stamp, Arch arch)
{
  Arena *arena = arena_alloc();
  OBJ *obj        = push_array(arena, OBJ, 1);
  obj->arena      = arena;
  obj->time_stamp = time_stamp;
  obj->arch       = arch;
  return obj;
}

internal void
obj_release(OBJ **obj_ptr)
{
  arena_release((*obj_ptr)->arena);
  *obj_ptr = 0;
}

internal OBJ_Symbol *
obj_push_symbol(OBJ *obj, String8 name, OBJ_SymbolScope scope, OBJ_RefKind ref_kind, void *ref)
{
  OBJ_SymbolNode *n = push_array(obj->arena, OBJ_SymbolNode, 1);
  SLLQueuePush(obj->symbols.first, obj->symbols.last, n);
  obj->symbols.count += 1;

  OBJ_Symbol *v = &n->v;
  v->name     = push_str8_copy(obj->arena, name);
  v->scope    = scope;
  v->ref_kind = ref_kind;
  v->ref      = ref;

  return v;
}

internal OBJ_Section *
obj_push_section(OBJ *obj, String8 name, OBJ_SectionFlags flags)
{
  OBJ_SectionNode *n = push_array(obj->arena, OBJ_SectionNode, 1);
  SLLQueuePush(obj->sections.first, obj->sections.last, n);
  obj->sections.count += 1;

  OBJ_Section *v = &n->v;
  v->name   = push_str8_copy(obj->arena, name);
  v->flags  = flags;
  v->symbol = obj_push_symbol(obj, name, OBJ_SymbolScope_Local, OBJ_RefKind_Section, v);
  str8_serial_begin(obj->arena, &v->data);

  return v;
}

internal OBJ_Reloc *
obj_push_reloc(OBJ *obj, OBJ_Section *sec, OBJ_RelocKind kind, U64 offset, OBJ_Symbol *symbol)
{
  OBJ_RelocNode *n = push_array(obj->arena, OBJ_RelocNode, 1);
  SLLQueuePush(sec->relocs.first, sec->relocs.last, n);
  sec->relocs.count += 1;

  OBJ_Reloc *v = &n->v;
  v->kind   = kind;
  v->offset = offset;
  v->symbol = symbol;

  return v;
}

