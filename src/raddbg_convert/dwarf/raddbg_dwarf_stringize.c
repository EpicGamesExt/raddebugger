// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ DWARF Stringize Functions

static char dwarf_spaces[] = "                ";

static void
dwarf_stringize_info(Arena *arena, String8List *out, DWARF_InfoUnit *unit, U32 indent){
  String8 unit_type_string = dwarf_string_from_unit_type((DWARF_UnitType)unit->unit_type);
  
  str8_list_pushf(arena, out, "%.*shdr_off=0x%llx\n", indent, dwarf_spaces, unit->hdr_off);
  str8_list_pushf(arena, out, "%.*sbase_off=0x%llx\n", indent, dwarf_spaces, unit->base_off);
  str8_list_pushf(arena, out, "%.*sopl_off=0x%llx\n", indent, dwarf_spaces, unit->opl_off);
  str8_list_pushf(arena, out, "%.*soffset_size=%u\n", indent, dwarf_spaces,
                  unit->offset_size);
  str8_list_pushf(arena, out, "%.*sversion=%u\n", indent, dwarf_spaces, unit->version);
  str8_list_pushf(arena, out, "%.*sunit_type=%.*s\n", indent, dwarf_spaces,
                  str8_varg(unit_type_string));
  str8_list_pushf(arena, out, "%.*saddress_size=%u\n", indent, dwarf_spaces,
                  unit->address_size);
  str8_list_pushf(arena, out, "%.*sabbrev_off=0x%llx\n", indent, dwarf_spaces,
                  unit->abbrev_off);
  
  switch (unit->unit_type){
    case DWARF_UnitType_skeleton: case DWARF_UnitType_split_compile:
    {
      str8_list_pushf(arena, out, "%.*sdwo_id=%llu\n", indent, dwarf_spaces, unit->dwo_id);
    }break;
    
    case DWARF_UnitType_type: case DWARF_UnitType_split_type:
    {
      str8_list_pushf(arena, out, "%.*stype_signature=%llu\n", indent, dwarf_spaces,
                      unit->type_signature);
      str8_list_pushf(arena, out, "%.*stype_offset=%llu\n", indent, dwarf_spaces,
                      unit->type_offset);
    }break;
  }
}

static void
dwarf_stringize_pubnames(Arena *arena, String8List *out, DWARF_PubNamesUnit *unit,
                         U32 indent){
  str8_list_pushf(arena, out, "%.*shdr_off=0x%llx\n", indent, dwarf_spaces, unit->hdr_off);
  str8_list_pushf(arena, out, "%.*sbase_off=0x%llx\n", indent, dwarf_spaces, unit->base_off);
  str8_list_pushf(arena, out, "%.*sopl_off=0x%llx\n", indent, dwarf_spaces, unit->opl_off);
  str8_list_pushf(arena, out, "%.*soffset_size=%u\n", indent, dwarf_spaces, unit->offset_size);
  str8_list_pushf(arena, out, "%.*sversion=%u\n", indent, dwarf_spaces, unit->version);
  str8_list_pushf(arena, out, "%.*sinfo_off=0x%llx\n", indent, dwarf_spaces, unit->info_off);
  str8_list_pushf(arena, out, "%.*sinfo_length=0x%llx\n", indent, dwarf_spaces,
                  unit->info_length);
}

static void
dwarf_stringize_names(Arena *arena, String8List *out, DWARF_NamesUnit *unit, U32 indent){
  str8_list_pushf(arena, out, "%.*shdr_off=0x%llx\n", indent, dwarf_spaces, unit->hdr_off);
  str8_list_pushf(arena, out, "%.*sbase_off=0x%llx\n", indent, dwarf_spaces, unit->base_off);
  str8_list_pushf(arena, out, "%.*sopl_off=0x%llx\n", indent, dwarf_spaces, unit->opl_off);
  str8_list_pushf(arena, out, "%.*sversion=%u\n", indent, dwarf_spaces, unit->version);
  str8_list_pushf(arena, out, "%.*scomp_unit_count=%u\n", indent, dwarf_spaces,
                  unit->comp_unit_count);
  str8_list_pushf(arena, out, "%.*slocal_type_unit_count=%u\n", indent, dwarf_spaces,
                  unit->local_type_unit_count);
  str8_list_pushf(arena, out, "%.*sforeign_type_unit_count=%u\n", indent, dwarf_spaces,
                  unit->foreign_type_unit_count);
  str8_list_pushf(arena, out, "%.*sbucket_count=%u\n", indent, dwarf_spaces,
                  unit->bucket_count);
  str8_list_pushf(arena, out, "%.*sname_count=%u\n", indent, dwarf_spaces, unit->name_count);
  str8_list_pushf(arena, out, "%.*sabbrev_table_size=%u\n", indent, dwarf_spaces,
                  unit->abbrev_table_size);
  str8_list_pushf(arena, out, "%.*saugmentation_string=%.*s\n", indent, dwarf_spaces,
                  str8_varg(unit->augmentation_string));
}

static void
dwarf_stringize_aranges(Arena *arena, String8List *out, DWARF_ArangesUnit *unit, U32 indent){
  str8_list_pushf(arena, out, "%.*shdr_off=0x%llx\n", indent, dwarf_spaces, unit->hdr_off);
  str8_list_pushf(arena, out, "%.*sbase_off=0x%llx\n", indent, dwarf_spaces, unit->base_off);
  str8_list_pushf(arena, out, "%.*sopl_off=0x%llx\n", indent, dwarf_spaces, unit->opl_off);
  str8_list_pushf(arena, out, "%.*sversion=%u\n", indent, dwarf_spaces, unit->version);
  str8_list_pushf(arena, out, "%.*saddress_size=%u\n", indent, dwarf_spaces,
                  unit->address_size);
  str8_list_pushf(arena, out, "%.*ssegment_selector_size=%u\n", indent, dwarf_spaces,
                  unit->segment_selector_size);
  str8_list_pushf(arena, out, "%.*soffset_size=%u\n", indent, dwarf_spaces, unit->offset_size);
  str8_list_pushf(arena, out, "%.*sinfo_off=0x%llx\n", indent, dwarf_spaces, unit->info_off);
}

static void
dwarf_stringize_addr(Arena *arena, String8List *out, DWARF_AddrUnit *unit, U32 indent){
  str8_list_pushf(arena, out, "%.*shdr_off=0x%llx\n", indent, dwarf_spaces, unit->hdr_off);
  str8_list_pushf(arena, out, "%.*sbase_off=0x%llx\n", indent, dwarf_spaces, unit->base_off);
  str8_list_pushf(arena, out, "%.*sopl_off=0x%llx\n", indent, dwarf_spaces, unit->opl_off);
  str8_list_pushf(arena, out, "%.*soffset_size=%u\n", indent, dwarf_spaces,
                  unit->offset_size);
  str8_list_pushf(arena, out, "%.*sversion=%u\n", indent, dwarf_spaces, unit->dwarf_version);
  str8_list_pushf(arena, out, "%.*saddress_size=%u\n", indent, dwarf_spaces,
                  unit->address_size);
  str8_list_pushf(arena, out, "%.*ssegment_selector_size=%u\n", indent, dwarf_spaces,
                  unit->segment_selector_size);
}
