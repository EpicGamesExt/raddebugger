// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_GFX_MAC_H
#define OS_GFX_MAC_H

////////////////////////////////
//~ rjf: Includes

// objc-runtime redefines nil in the headers
#define nil nil
#undef internal
#define os_release os_release_object

#include <CoreGraphics/CoreGraphics.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc-runtime.h>

#undef nil
#define internal static
#undef os_release

////////////////////////////////
//~ rjf: Window State

typedef struct OS_MAC_TitleBarClientArea OS_MAC_TitleBarClientArea;
struct OS_MAC_TitleBarClientArea
{
  OS_MAC_TitleBarClientArea *next;
  Rng2F32 rect;
};

typedef struct OS_MAC_Window OS_MAC_Window;
struct OS_MAC_Window
{
  OS_MAC_Window *next;
  OS_MAC_Window *prev;
  id win;
  B32 custom_border;
  F32 custom_border_title_thickness;
  B32 dragging_window;
  Arena *paint_arena;
  OS_MAC_TitleBarClientArea *first_title_bar_client_area;
  OS_MAC_TitleBarClientArea *last_title_bar_client_area;
};

////////////////////////////////
//~ rjf: Key State

typedef U8 OS_MAC_KeyState;
enum {
  OS_MAC_KeyState_None = 0,
  OS_MAC_KeyState_Down = 1,
};

////////////////////////////////
//~ rjf: State Bundle

typedef struct OS_MAC_GfxState OS_MAC_GfxState;
struct OS_MAC_GfxState
{
  Arena *arena;
  OS_MAC_Window *first_window;
  OS_MAC_Window *last_window;
  OS_MAC_Window *free_window;
  OS_MAC_KeyState keys[OS_Key_COUNT];
  OS_Modifiers modifiers;
  id cursors[OS_Cursor_COUNT];
  B32 all_windows_closed;
  OS_GfxInfo gfx_info;
};

////////////////////////////////
//~ rjf: Globals

global OS_MAC_GfxState *os_mac_gfx_state = 0;

////////////////////////////////
//~ rjf: Helpers

#define msg(r, o, s) ((r (*)(id, SEL))objc_msgSend)(o, sel_getUid(s))
#define msg1(r, o, s, A, a) \
    ((r (*)(id, SEL, A))objc_msgSend)(o, sel_getUid(s), a)
#define msg2(r, o, s, A, a, B, b) \
    ((r (*)(id, SEL, A, B))objc_msgSend)(o, sel_getUid(s), a, b)
#define msg3(r, o, s, A, a, B, b, C, c) \
    ((r (*)(id, SEL, A, B, C))objc_msgSend)(o, sel_getUid(s), a, b, c)
#define msg4(r, o, s, A, a, B, b, C, c, D, d) \
    ((r (*)(id, SEL, A, B, C, D))objc_msgSend)(o, sel_getUid(s), a, b, c, d)

#define cls(x) ((id)objc_getClass(x))

internal OS_Event *
os_mac_gfx_event_list_push_key(Arena* arena, OS_EventList *evts, OS_MAC_Window* os_window,  OS_Key key, B32 is_down);

#endif // OS_GFX_MAC_H
