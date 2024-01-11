// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
// NOTE(allen): Dev Arena

#if ENABLE_DEV

internal void
arena_annotate_push__dev(Arena *arena, U64 size, void *ptr){
  ArenaDev *dev = arena->dev;
  if (dev != 0 && ptr != 0){
    //- read location
    char *file_name = 0;
    U64 line_number = 0;
    tctx_read_srcloc(&file_name, &line_number);
    tctx_write_srcloc(0, 0);
    
    //- profile
    ArenaProf *prof = dev->prof;
    if (prof != 0){
      // c string -> string
      String8 file_name_str = str8_lit("(null)");
      if (file_name != 0){
        file_name_str = str8_cstring(file_name);
      }
      // record
      arena_prof_inc_counters__dev(dev->arena, prof, file_name_str, line_number, size, 1);
    }
  }
}

internal void
arena_annotate_absorb__dev(Arena *arena, Arena *sub){
  ArenaDev *dev = arena->dev;
  ArenaDev *sub_dev = sub->dev;
  if (dev != 0 && sub_dev != 0){
    //- merge profiles
    ArenaProf *prof = dev->prof;
    ArenaProf *sub_prof = sub_dev->prof;
    if (prof != 0 && sub_prof != 0){
      for (ArenaProfNode *sub_node = sub_prof->first;
           sub_node != 0;
           sub_node = sub_node->next){
        arena_prof_inc_counters__dev(dev->arena, prof, sub_node->file_name, sub_node->line,
                                     sub_node->size, sub_node->count);
      }
    }
  }
  //- release the sub dev memory
  if (sub_dev != 0){
    arena_release(sub_dev->arena);
  }
}

internal ArenaDev*
arena_equip__dev(Arena *arena){
  ArenaDev *result = arena->dev;
  if (result == 0){
    Arena *dev_arena = arena_alloc();
    ArenaDev *dev = (ArenaDev*)arena_push__impl(dev_arena, sizeof(ArenaDev));
    MemoryZeroStruct(dev);
    dev->arena = dev_arena;
    arena->dev = dev;
    result = dev;
  }
  return(result);
}

internal void
arena_equip_profile__dev(Arena *arena){
  ArenaDev *dev = arena_equip__dev(arena);
  if (dev->prof == 0){
    dev->prof = (ArenaProf*)arena_push__impl(dev->arena, sizeof(ArenaProf));
    MemoryZeroStruct(dev->prof);
  }
}

internal void
arena_print_profile__dev(Arena *arena, Arena *out_arena, String8List *out){
  Assert(arena != out_arena);
  
  //- get dev & disable
  ArenaDev *dev = arena->dev;
  arena->dev = 0;
  
  //- get prof
  ArenaProf *prof = (dev != 0)?dev->prof:0;
  
  //- not equipped with prof
  if (prof == 0){
    str8_list_push(out_arena, out, str8_lit("not equipped with a memory profile\n"));
  }
  
  //- print prof
  if (prof != 0){
    Temp scratch = temp_begin(dev->arena);
    
    //- make flat array
    U64 note_count = prof->count;
    ArenaProfNode **notes = push_array_no_zero__no_annotation(scratch.arena, ArenaProfNode*, note_count);
    {
      ArenaProfNode **note_ptr = notes;
      for (ArenaProfNode *node = prof->first;
           node != 0;
           node = node->next, note_ptr += 1){
        *note_ptr = node;
      }
    }
    
    //- file name size
    U64 max_file_name_size = 0;
    {
      ArenaProfNode **note_ptr = notes;
      for (U64 i = 0; i < note_count; i += 1, note_ptr += 1){
        max_file_name_size = Max(max_file_name_size, (**note_ptr).file_name.size);
      }
    }
    
    //- sort (> size, < [address])
    for (U64 i = 0; i < note_count; i += 1){
      ArenaProfNode **i_note = notes + i;
      ArenaProfNode **min_note = i_note;
      for (U64 j = i + 1; j < note_count; j += 1){
        ArenaProfNode **j_note = notes + j;
        if ((**j_note).size > (**min_note).size ||
            ((**j_note).size == (**min_note).size && *j_note < *min_note)){
          min_note = j_note;
        }
      }
      if (min_note != i_note){
        ArenaProfNode *t = *i_note;
        *i_note = *min_note;
        *min_note = t;
      }
    }
    
    //- total size
    U64 total_size = 0;
    {
      ArenaProfNode **note_ptr = notes;
      for (U64 i = 0; i < note_count; i += 1, note_ptr += 1){
        ArenaProfNode *note = *note_ptr;
        total_size += note->size;
      }
    }
    
    //- print
    {
      str8_list_pushf(out_arena, out, "memory total: %llu\n", total_size);
      
      ArenaProfNode **note_ptr = notes;
      for (U64 i = 0; i < note_count; i += 1, note_ptr += 1){
        ArenaProfNode *note = *note_ptr;
        String8 location = push_str8f(scratch.arena, "%S:%5llu:",
                                      note->file_name, note->line);
        F32 percent = 100.f*((F32)note->size)/total_size;
        str8_list_pushf(out_arena, out, "%*.*s %12llu %5.2f%% [%5llu]\n",
                        max_file_name_size + 7, str8_varg(location),
                        note->size, percent, note->count);
      }
    }
    
    temp_end(scratch);
  }
  
  //- restore dev
  arena->dev = dev;
}

internal void
arena_prof_inc_counters__dev(Arena *dev_arena, ArenaProf *prof, String8 file_name, U64 line,
                             U64 size, U64 count){
  // find existing profile node
  ArenaProfNode *prof_node = 0;
  for (ArenaProfNode *node = prof->first;
       node != 0;
       node = node->next){
    if (node->line == line && str8_match(file_name, node->file_name, 0)){
      prof_node = node;
      break;
    }
  }
  // make new histogram node if necessary
  if (prof_node == 0){
    prof_node = (ArenaProfNode*)arena_push(dev_arena, sizeof(*prof_node));
    SLLQueuePush(prof->first, prof->last, prof_node);
    prof->count += 1;
    prof_node->file_name = file_name;
    prof_node->line = line;
  }
  // record this allocation
  prof_node->size += size;
  prof_node->count += count;
}

#endif
