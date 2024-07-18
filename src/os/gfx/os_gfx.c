// Copyright (c) 2024 Epic Games Tools
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
os_string_list_from_event_flags(Arena *arena, OS_EventFlags flags)
{
  String8List result = {0};
  String8 flag_strs[] = 
  {
    str8_lit("Ctrl"),
    str8_lit("Shift"),
    str8_lit("Alt"),
  };
  str8_list_from_flags(arena, &result, flags, flag_strs, ArrayCount(flag_strs));
  return result;
}

internal U32
os_codepoint_from_event_flags_and_key(OS_EventFlags flags, OS_Key key)
{
  U32 result = 0;
  
  // rjf: special-case map
  local_persist read_only struct {U32 character; OS_Key key; OS_EventFlags flags;} map[] =
  {
    {'!', OS_Key_1, OS_EventFlag_Shift},
    {'@', OS_Key_2, OS_EventFlag_Shift},
    {'#', OS_Key_3, OS_EventFlag_Shift},
    {'$', OS_Key_4, OS_EventFlag_Shift},
    {'%', OS_Key_5, OS_EventFlag_Shift},
    {'^', OS_Key_6, OS_EventFlag_Shift},
    {'&', OS_Key_7, OS_EventFlag_Shift},
    {'*', OS_Key_8, OS_EventFlag_Shift},
    {'(', OS_Key_9, OS_EventFlag_Shift},
    {')', OS_Key_0, OS_EventFlag_Shift},
    {'_', OS_Key_Minus, OS_EventFlag_Shift},
    {'_', OS_Key_Minus, OS_EventFlag_Shift},
    {'-', OS_Key_Minus, 0},
    {'=', OS_Key_Equal, 0},
    {'+', OS_Key_Equal, OS_EventFlag_Shift},
    {'`', OS_Key_Tick, 0},
    {'~', OS_Key_Tick, OS_EventFlag_Shift},
    {'[', OS_Key_LeftBracket, 0},
    {']', OS_Key_RightBracket, 0},
    {'{', OS_Key_LeftBracket, OS_EventFlag_Shift},
    {'}', OS_Key_RightBracket, OS_EventFlag_Shift},
    {'\\', OS_Key_BackSlash, 0},
    {'|', OS_Key_BackSlash, OS_EventFlag_Shift},
    {';', OS_Key_Semicolon, 0},
    {':', OS_Key_Semicolon, OS_EventFlag_Shift},
    {'\'', OS_Key_Quote, 0},
    {'"', OS_Key_Quote, OS_EventFlag_Shift},
    {'.', OS_Key_Period, 0},
    {',', OS_Key_Comma, 0},
    {'<', OS_Key_Period, OS_EventFlag_Shift},
    {'>', OS_Key_Comma, OS_EventFlag_Shift},
    {'/', OS_Key_Slash, 0},
    {'?', OS_Key_Slash, OS_EventFlag_Shift},
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
    {'A', OS_Key_A, OS_EventFlag_Shift},
    {'B', OS_Key_B, OS_EventFlag_Shift},
    {'C', OS_Key_C, OS_EventFlag_Shift},
    {'D', OS_Key_D, OS_EventFlag_Shift},
    {'E', OS_Key_E, OS_EventFlag_Shift},
    {'F', OS_Key_F, OS_EventFlag_Shift},
    {'G', OS_Key_G, OS_EventFlag_Shift},
    {'H', OS_Key_H, OS_EventFlag_Shift},
    {'I', OS_Key_I, OS_EventFlag_Shift},
    {'J', OS_Key_J, OS_EventFlag_Shift},
    {'K', OS_Key_K, OS_EventFlag_Shift},
    {'L', OS_Key_L, OS_EventFlag_Shift},
    {'M', OS_Key_M, OS_EventFlag_Shift},
    {'N', OS_Key_N, OS_EventFlag_Shift},
    {'O', OS_Key_O, OS_EventFlag_Shift},
    {'P', OS_Key_P, OS_EventFlag_Shift},
    {'Q', OS_Key_Q, OS_EventFlag_Shift},
    {'R', OS_Key_R, OS_EventFlag_Shift},
    {'S', OS_Key_S, OS_EventFlag_Shift},
    {'T', OS_Key_T, OS_EventFlag_Shift},
    {'U', OS_Key_U, OS_EventFlag_Shift},
    {'V', OS_Key_V, OS_EventFlag_Shift},
    {'W', OS_Key_W, OS_EventFlag_Shift},
    {'X', OS_Key_X, OS_EventFlag_Shift},
    {'Y', OS_Key_Y, OS_EventFlag_Shift},
    {'Z', OS_Key_Z, OS_EventFlag_Shift},
  };
  
  // rjf: check numeric
  if(OS_Key_0 <= key && key <= OS_Key_9)
  {
    result = '0' + (key - OS_Key_0);
  }
  
  // rjf: check special-case map
  for(U64 idx = 0; idx < ArrayCount(map); idx += 1)
  {
    if(map[idx].key == key && map[idx].flags == flags)
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
os_key_press(OS_EventList *events, OS_Handle window, OS_EventFlags flags, OS_Key key)
{
  B32 result = 0;
  for(OS_Event *event = events->first; event != 0; event = event->next)
  {
    if((os_handle_match(event->window, window) || os_handle_match(window, os_handle_zero())) &&
       event->kind == OS_EventKind_Press && event->key == key && event->flags == flags)
    {
      result = 1;
      os_eat_event(events, event);
      break;
    }
  }
  return result;
}

internal B32
os_key_release(OS_EventList *events, OS_Handle window, OS_EventFlags flags, OS_Key key)
{
  B32 result = 0;
  for(OS_Event *event = events->first; event != 0; event = event->next)
  {
    if((os_handle_match(event->window, window) || os_handle_match(window, os_handle_zero())) &&
       event->kind == OS_EventKind_Release && event->key == key && event->flags == flags)
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
