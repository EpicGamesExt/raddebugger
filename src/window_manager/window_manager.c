// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "window_manager/generated/window_manager.meta.c"

////////////////////////////////
//~ rjf: Handle Type Helpers

internal WM_Window
wm_window_zero(void)
{
  WM_Window result = {0};
  return result;
}

internal B32
wm_window_match(WM_Window a, WM_Window b)
{
  B32 result = MemoryMatchStruct(&a, &b);
  return result;
}

internal B32
wm_monitor_match(WM_Monitor a, WM_Monitor b)
{
  B32 result = MemoryMatchStruct(&a, &b);
  return result;
}

////////////////////////////////
//~ rjf: Event Functions (Helpers, Implemented Once)

internal String8
wm_string_from_event_kind(WM_EventKind kind)
{
  String8 result = {0};
  switch(kind)
  {
    case WM_EventKind_Null:
    case WM_EventKind_COUNT:
    {}break;
    case WM_EventKind_Press:                {result = str8_lit("Press");}break;
    case WM_EventKind_Release:              {result = str8_lit("Release");}break;
    case WM_EventKind_MouseMove:            {result = str8_lit("MouseMove");}break;
    case WM_EventKind_Text:                 {result = str8_lit("Text");}break;
    case WM_EventKind_Scroll:               {result = str8_lit("Scroll");}break;
    case WM_EventKind_WindowLoseFocus:      {result = str8_lit("WindowLoseFocus");}break;
    case WM_EventKind_WindowClose:          {result = str8_lit("WindowClose");}break;
    case WM_EventKind_FileDrop:             {result = str8_lit("FileDrop");}break;
    case WM_EventKind_Wakeup:               {result = str8_lit("Wakeup");}break;
  }
  return result;
}

internal String8List
wm_string_list_from_modifiers(Arena *arena, WM_Modifiers modifiers)
{
  String8List result = {0};
  if(modifiers & WM_Modifier_Ctrl)  { str8_list_push(arena, &result, str8_lit("Ctrl")); }
  if(modifiers & WM_Modifier_Shift) { str8_list_push(arena, &result, str8_lit("Shift")); }
  if(modifiers & WM_Modifier_Alt)   { str8_list_push(arena, &result, str8_lit("Alt")); }
  return result;
}

internal String8
wm_string_from_modifiers_key(Arena *arena, WM_Modifiers modifiers, WM_Key key)
{
  String8 result = {0};
  if(key != WM_Key_Null)
  {
    Temp scratch = scratch_begin(&arena, 1);
    String8List mods = wm_string_list_from_modifiers(scratch.arena, modifiers);
    String8 key_string = wm_key_display_name_table[key];
    str8_list_push(scratch.arena, &mods, key_string);
    StringJoin join = {0};
    join.sep = str8_lit(" + ");
    result = str8_list_join(arena, &mods, &join);
    scratch_end(scratch);
  }
  return result;
}

internal U32
wm_codepoint_from_modifiers_and_key(WM_Modifiers modifiers, WM_Key key)
{
  U32 result = 0;
  
  // rjf: special-case map
  local_persist read_only struct {U32 character; WM_Key key; WM_Modifiers modifiers;} map[] =
  {
    {'!', WM_Key_1, WM_Modifier_Shift},
    {'@', WM_Key_2, WM_Modifier_Shift},
    {'#', WM_Key_3, WM_Modifier_Shift},
    {'$', WM_Key_4, WM_Modifier_Shift},
    {'%', WM_Key_5, WM_Modifier_Shift},
    {'^', WM_Key_6, WM_Modifier_Shift},
    {'&', WM_Key_7, WM_Modifier_Shift},
    {'*', WM_Key_8, WM_Modifier_Shift},
    {'(', WM_Key_9, WM_Modifier_Shift},
    {')', WM_Key_0, WM_Modifier_Shift},
    {'_', WM_Key_Minus, WM_Modifier_Shift},
    {'_', WM_Key_Minus, WM_Modifier_Shift},
    {'-', WM_Key_Minus, 0},
    {'=', WM_Key_Equal, 0},
    {'+', WM_Key_Equal, WM_Modifier_Shift},
    {'`', WM_Key_Tick, 0},
    {'~', WM_Key_Tick, WM_Modifier_Shift},
    {'[', WM_Key_LeftBracket, 0},
    {']', WM_Key_RightBracket, 0},
    {'{', WM_Key_LeftBracket, WM_Modifier_Shift},
    {'}', WM_Key_RightBracket, WM_Modifier_Shift},
    {'\\', WM_Key_BackSlash, 0},
    {'|', WM_Key_BackSlash, WM_Modifier_Shift},
    {';', WM_Key_Semicolon, 0},
    {':', WM_Key_Semicolon, WM_Modifier_Shift},
    {'\'', WM_Key_Quote, 0},
    {'"', WM_Key_Quote, WM_Modifier_Shift},
    {'.', WM_Key_Period, 0},
    {',', WM_Key_Comma, 0},
    {'<', WM_Key_Period, WM_Modifier_Shift},
    {'>', WM_Key_Comma, WM_Modifier_Shift},
    {'/', WM_Key_Slash, 0},
    {'?', WM_Key_Slash, WM_Modifier_Shift},
    {'a', WM_Key_A, 0},
    {'b', WM_Key_B, 0},
    {'c', WM_Key_C, 0},
    {'d', WM_Key_D, 0},
    {'e', WM_Key_E, 0},
    {'f', WM_Key_F, 0},
    {'g', WM_Key_G, 0},
    {'h', WM_Key_H, 0},
    {'i', WM_Key_I, 0},
    {'j', WM_Key_J, 0},
    {'k', WM_Key_K, 0},
    {'l', WM_Key_L, 0},
    {'m', WM_Key_M, 0},
    {'n', WM_Key_N, 0},
    {'o', WM_Key_O, 0},
    {'p', WM_Key_P, 0},
    {'q', WM_Key_Q, 0},
    {'r', WM_Key_R, 0},
    {'s', WM_Key_S, 0},
    {'t', WM_Key_T, 0},
    {'u', WM_Key_U, 0},
    {'v', WM_Key_V, 0},
    {'w', WM_Key_W, 0},
    {'x', WM_Key_X, 0},
    {'y', WM_Key_Y, 0},
    {'z', WM_Key_Z, 0},
    {'A', WM_Key_A, WM_Modifier_Shift},
    {'B', WM_Key_B, WM_Modifier_Shift},
    {'C', WM_Key_C, WM_Modifier_Shift},
    {'D', WM_Key_D, WM_Modifier_Shift},
    {'E', WM_Key_E, WM_Modifier_Shift},
    {'F', WM_Key_F, WM_Modifier_Shift},
    {'G', WM_Key_G, WM_Modifier_Shift},
    {'H', WM_Key_H, WM_Modifier_Shift},
    {'I', WM_Key_I, WM_Modifier_Shift},
    {'J', WM_Key_J, WM_Modifier_Shift},
    {'K', WM_Key_K, WM_Modifier_Shift},
    {'L', WM_Key_L, WM_Modifier_Shift},
    {'M', WM_Key_M, WM_Modifier_Shift},
    {'N', WM_Key_N, WM_Modifier_Shift},
    {'O', WM_Key_O, WM_Modifier_Shift},
    {'P', WM_Key_P, WM_Modifier_Shift},
    {'Q', WM_Key_Q, WM_Modifier_Shift},
    {'R', WM_Key_R, WM_Modifier_Shift},
    {'S', WM_Key_S, WM_Modifier_Shift},
    {'T', WM_Key_T, WM_Modifier_Shift},
    {'U', WM_Key_U, WM_Modifier_Shift},
    {'V', WM_Key_V, WM_Modifier_Shift},
    {'W', WM_Key_W, WM_Modifier_Shift},
    {'X', WM_Key_X, WM_Modifier_Shift},
    {'Y', WM_Key_Y, WM_Modifier_Shift},
    {'Z', WM_Key_Z, WM_Modifier_Shift},
    {' ', WM_Key_Space, 0},
  };
  
  // rjf: check numeric
  if(WM_Key_0 <= key && key <= WM_Key_9)
  {
    result = '0' + (key - WM_Key_0);
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
wm_eat_event(WM_EventList *events, WM_Event *event)
{
  DLLRemove(events->first, events->last, event);
  events->count -= 1;
}

internal B32
wm_key_press(WM_EventList *events, WM_Window window, WM_Modifiers modifiers, WM_Key key)
{
  B32 result = 0;
  for(WM_Event *event = events->first; event != 0; event = event->next)
  {
    if((wm_window_match(event->window, window) || wm_window_match(window, wm_window_zero())) &&
       event->kind == WM_EventKind_Press && event->key == key && event->modifiers == modifiers)
    {
      result = 1;
      wm_eat_event(events, event);
      break;
    }
  }
  return result;
}

internal B32
wm_key_release(WM_EventList *events, WM_Window window, WM_Modifiers modifiers, WM_Key key)
{
  B32 result = 0;
  for(WM_Event *event = events->first; event != 0; event = event->next)
  {
    if((wm_window_match(event->window, window) || wm_window_match(window, wm_window_zero())) &&
       event->kind == WM_EventKind_Release && event->key == key && event->modifiers == modifiers)
    {
      result = 1;
      wm_eat_event(events, event);
      break;
    }
  }
  return result;
}

internal B32
wm_text(WM_EventList *events, WM_Window window, U32 character)
{
  B32 result = 0;
  for(WM_Event *event = events->first; event != 0; event = event->next)
  {
    if((wm_window_match(event->window, window) || wm_window_match(window, wm_window_zero())) &&
       event->kind == WM_EventKind_Text && event->character == character)
    {
      result = 1;
      wm_eat_event(events, event);
      break;
    }
  }
  return result;
}

internal WM_EventList
wm_event_list_copy(Arena *arena, WM_EventList *src)
{
  WM_EventList dst = {0};
  for(WM_Event *s = src->first; s != 0; s = s->next)
  {
    WM_Event *d = push_array(arena, WM_Event, 1);
    MemoryCopyStruct(d, s);
    d->strings = str8_list_copy(arena, &s->strings);
    DLLPushBack(dst.first, dst.last, d);
    dst.count += 1;
  }
  return dst;
}

internal void
wm_event_list_concat_in_place(WM_EventList *dst, WM_EventList *to_push)
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

internal WM_Event *
wm_event_list_push_new(Arena *arena, WM_EventList *evts, WM_EventKind kind)
{
  WM_Event *evt = push_array(arena, WM_Event, 1);
  DLLPushBack(evts->first, evts->last, evt);
  evts->count += 1;
  evt->timestamp_us = now_time_us();
  evt->kind = kind;
  return evt;
}
