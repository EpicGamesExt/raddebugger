// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "generated/os_gfx.meta.c"

////////////////////////////////
//~ rjf: Event Functions (Helpers, Implemented Once)

internal String8
os_string_from_event_kind(OS_EventKind kind)
{
  String8 result = {0};
  switch(kind)
  {
    case OS_EventKind_Null:
    case OS_EventKind_COUNT:
    {}break;
    case OS_EventKind_Press:                {result = str8_lit("Press");}break;
    case OS_EventKind_Release:              {result = str8_lit("Release");}break;
    case OS_EventKind_MouseMove:            {result = str8_lit("MouseMove");}break;
    case OS_EventKind_Text:                 {result = str8_lit("Text");}break;
    case OS_EventKind_Scroll:               {result = str8_lit("Scroll");}break;
    case OS_EventKind_WindowLoseFocus:      {result = str8_lit("WindowLoseFocus");}break;
    case OS_EventKind_WindowClose:          {result = str8_lit("WindowClose");}break;
    case OS_EventKind_FileDrop:             {result = str8_lit("FileDrop");}break;
    case OS_EventKind_Wakeup:               {result = str8_lit("Wakeup");}break;
  }
  return result;
}

internal String8List
os_string_list_from_modifiers(Arena *arena, OS_Modifiers modifiers)
{
  String8List result = {0};
  if(modifiers & OS_Modifier_Ctrl)  { str8_list_push(arena, &result, str8_lit("Ctrl")); }
  if(modifiers & OS_Modifier_Shift) { str8_list_push(arena, &result, str8_lit("Shift")); }
  if(modifiers & OS_Modifier_Alt)   { str8_list_push(arena, &result, str8_lit("Alt")); }
  return result;
}

internal String8
os_string_from_modifiers_key(Arena *arena, OS_Modifiers modifiers, OS_Key key)
{
  String8 result = {0};
  if(key != OS_Key_Null)
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8List mods = os_string_list_from_modifiers(scratch.arena, modifiers);
    String8 key_string = os_g_key_display_string_table[key];
    str8_list_push(scratch.arena, &mods, key_string);
    StringJoin join = {0};
    join.sep = str8_lit(" + ");
    result = str8_list_join(arena, &mods, &join);
    scratch_end(scratch);
  }
  return result;
}

internal U32
os_codepoint_from_modifiers_and_key(OS_Modifiers modifiers, OS_Key key)
{
  U32 result = 0;
  
  // rjf: special-case map
  local_persist read_only struct {U32 character; OS_Key key; OS_Modifiers modifiers;} map[] =
  {
    {'!', OS_Key_1, OS_Modifier_Shift},
    {'@', OS_Key_2, OS_Modifier_Shift},
    {'#', OS_Key_3, OS_Modifier_Shift},
    {'$', OS_Key_4, OS_Modifier_Shift},
    {'%', OS_Key_5, OS_Modifier_Shift},
    {'^', OS_Key_6, OS_Modifier_Shift},
    {'&', OS_Key_7, OS_Modifier_Shift},
    {'*', OS_Key_8, OS_Modifier_Shift},
    {'(', OS_Key_9, OS_Modifier_Shift},
    {')', OS_Key_0, OS_Modifier_Shift},
    {'_', OS_Key_Minus, OS_Modifier_Shift},
    {'_', OS_Key_Minus, OS_Modifier_Shift},
    {'-', OS_Key_Minus, 0},
    {'=', OS_Key_Equal, 0},
    {'+', OS_Key_Equal, OS_Modifier_Shift},
    {'`', OS_Key_Tick, 0},
    {'~', OS_Key_Tick, OS_Modifier_Shift},
    {'[', OS_Key_LeftBracket, 0},
    {']', OS_Key_RightBracket, 0},
    {'{', OS_Key_LeftBracket, OS_Modifier_Shift},
    {'}', OS_Key_RightBracket, OS_Modifier_Shift},
    {'\\', OS_Key_BackSlash, 0},
    {'|', OS_Key_BackSlash, OS_Modifier_Shift},
    {';', OS_Key_Semicolon, 0},
    {':', OS_Key_Semicolon, OS_Modifier_Shift},
    {'\'', OS_Key_Quote, 0},
    {'"', OS_Key_Quote, OS_Modifier_Shift},
    {'.', OS_Key_Period, 0},
    {',', OS_Key_Comma, 0},
    {'<', OS_Key_Period, OS_Modifier_Shift},
    {'>', OS_Key_Comma, OS_Modifier_Shift},
    {'/', OS_Key_Slash, 0},
    {'?', OS_Key_Slash, OS_Modifier_Shift},
    {'a', OS_Key_A, 0},
    {'b', OS_Key_B, 0},
    {'c', OS_Key_C, 0},
    {'d', OS_Key_D, 0},
    {'e', OS_Key_E, 0},
    {'f', OS_Key_F, 0},
    {'g', OS_Key_G, 0},
    {'h', OS_Key_H, 0},
    {'i', OS_Key_I, 0},
    {'j', OS_Key_J, 0},
    {'k', OS_Key_K, 0},
    {'l', OS_Key_L, 0},
    {'m', OS_Key_M, 0},
    {'n', OS_Key_N, 0},
    {'o', OS_Key_O, 0},
    {'p', OS_Key_P, 0},
    {'q', OS_Key_Q, 0},
    {'r', OS_Key_R, 0},
    {'s', OS_Key_S, 0},
    {'t', OS_Key_T, 0},
    {'u', OS_Key_U, 0},
    {'v', OS_Key_V, 0},
    {'w', OS_Key_W, 0},
    {'x', OS_Key_X, 0},
    {'y', OS_Key_Y, 0},
    {'z', OS_Key_Z, 0},
    {'A', OS_Key_A, OS_Modifier_Shift},
    {'B', OS_Key_B, OS_Modifier_Shift},
    {'C', OS_Key_C, OS_Modifier_Shift},
    {'D', OS_Key_D, OS_Modifier_Shift},
    {'E', OS_Key_E, OS_Modifier_Shift},
    {'F', OS_Key_F, OS_Modifier_Shift},
    {'G', OS_Key_G, OS_Modifier_Shift},
    {'H', OS_Key_H, OS_Modifier_Shift},
    {'I', OS_Key_I, OS_Modifier_Shift},
    {'J', OS_Key_J, OS_Modifier_Shift},
    {'K', OS_Key_K, OS_Modifier_Shift},
    {'L', OS_Key_L, OS_Modifier_Shift},
    {'M', OS_Key_M, OS_Modifier_Shift},
    {'N', OS_Key_N, OS_Modifier_Shift},
    {'O', OS_Key_O, OS_Modifier_Shift},
    {'P', OS_Key_P, OS_Modifier_Shift},
    {'Q', OS_Key_Q, OS_Modifier_Shift},
    {'R', OS_Key_R, OS_Modifier_Shift},
    {'S', OS_Key_S, OS_Modifier_Shift},
    {'T', OS_Key_T, OS_Modifier_Shift},
    {'U', OS_Key_U, OS_Modifier_Shift},
    {'V', OS_Key_V, OS_Modifier_Shift},
    {'W', OS_Key_W, OS_Modifier_Shift},
    {'X', OS_Key_X, OS_Modifier_Shift},
    {'Y', OS_Key_Y, OS_Modifier_Shift},
    {'Z', OS_Key_Z, OS_Modifier_Shift},
    {' ', OS_Key_Space, 0},
  };
  
  // rjf: check numeric
  if(OS_Key_0 <= key && key <= OS_Key_9)
  {
    result = '0' + (key - OS_Key_0);
  }
  
  // rjf: check special-case map
  for(U64 idx = 0; idx < ArrayCount(map); idx += 1)
  {
    if(map[idx].key == key && map[idx].modifiers == modifiers)
    {
      result = map[idx].character;
      break;
    }
  }
  
  return result;
}

internal void
os_eat_event(OS_EventList *events, OS_Event *event)
{
  DLLRemove(events->first, events->last, event);
  events->count -= 1;
}

internal B32
os_key_press(OS_EventList *events, OS_Handle window, OS_Modifiers modifiers, OS_Key key)
{
  B32 result = 0;
  for(OS_Event *event = events->first; event != 0; event = event->next)
  {
    if((os_handle_match(event->window, window) || os_handle_match(window, os_handle_zero())) &&
       event->kind == OS_EventKind_Press && event->key == key && event->modifiers == modifiers)
    {
      result = 1;
      os_eat_event(events, event);
      break;
    }
  }
  return result;
}

internal B32
os_key_release(OS_EventList *events, OS_Handle window, OS_Modifiers modifiers, OS_Key key)
{
  B32 result = 0;
  for(OS_Event *event = events->first; event != 0; event = event->next)
  {
    if((os_handle_match(event->window, window) || os_handle_match(window, os_handle_zero())) &&
       event->kind == OS_EventKind_Release && event->key == key && event->modifiers == modifiers)
    {
      result = 1;
      os_eat_event(events, event);
      break;
    }
  }
  return result;
}

internal B32
os_text(OS_EventList *events, OS_Handle window, U32 character)
{
  B32 result = 0;
  for(OS_Event *event = events->first; event != 0; event = event->next)
  {
    if((os_handle_match(event->window, window) || os_handle_match(window, os_handle_zero())) &&
       event->kind == OS_EventKind_Text && event->character == character)
    {
      result = 1;
      os_eat_event(events, event);
      break;
    }
  }
  return result;
}

internal OS_EventList
os_event_list_copy(Arena *arena, OS_EventList *src)
{
  OS_EventList dst = {0};
  for(OS_Event *s = src->first; s != 0; s = s->next)
  {
    OS_Event *d = push_array(arena, OS_Event, 1);
    MemoryCopyStruct(d, s);
    d->strings = str8_list_copy(arena, &s->strings);
    DLLPushBack(dst.first, dst.last, d);
    dst.count += 1;
  }
  return dst;
}

internal void
os_event_list_concat_in_place(OS_EventList *dst, OS_EventList *to_push)
{
  if(dst->last && to_push->first)
  {
    dst->last->next = to_push->first;
    to_push->first->prev = dst->last;
    dst->last = to_push->last;
    dst->count += to_push->count;
  }
  else if(!dst->last && to_push->first)
  {
    MemoryCopyStruct(dst, to_push);
  }
  MemoryZeroStruct(to_push);
}

internal OS_Event *
os_event_list_push_new(Arena *arena, OS_EventList *evts, OS_EventKind kind)
{
  OS_Event *evt = push_array(arena, OS_Event, 1);
  DLLPushBack(evts->first, evts->last, evt);
  evts->count += 1;
  evt->timestamp_us = os_now_microseconds();
  evt->kind = kind;
  return evt;
}
