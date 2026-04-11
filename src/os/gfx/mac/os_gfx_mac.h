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

#include "os_gfx_mac_keycode.h"
#include "os_gfx_mac_appkit.h"

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
  Arena *event_arena;
  OS_MAC_Window *first_window;
  OS_MAC_Window *last_window;
  OS_MAC_Window *free_window;
  OS_MAC_KeyState keys[OS_Key_COUNT];
  OS_EventList event_queue;
  OS_Modifiers modifiers;
  id cursors[OS_Cursor_COUNT];
  OS_GfxInfo gfx_info;
};

////////////////////////////////
//~ rjf: Globals

global OS_MAC_GfxState *os_mac_gfx_state = 0;

////////////////////////////////
//~ rjf: Helpers

//- rjf: objective-c runtime helpers
#define cls(x) ((id)objc_getClass(x))
#define msg(r, o, s) ((r (*)(id, SEL))objc_msgSend)(o, sel_getUid(s))
#define msg1(r, o, s, A, a) \
    ((r (*)(id, SEL, A))objc_msgSend)(o, sel_getUid(s), a)
#define msg2(r, o, s, A, a, B, b) \
    ((r (*)(id, SEL, A, B))objc_msgSend)(o, sel_getUid(s), a, b)
#define msg3(r, o, s, A, a, B, b, C, c) \
    ((r (*)(id, SEL, A, B, C))objc_msgSend)(o, sel_getUid(s), a, b, c)
#define msg4(r, o, s, A, a, B, b, C, c, D, d) \
    ((r (*)(id, SEL, A, B, C, D))objc_msgSend)(o, sel_getUid(s), a, b, c, d)

//- rjf: NSString helpers
internal id NSString_fromUTF8(String8 string);

//- rjf: titlebar button helpers
internal void os_gfx_max_move_window_button(id window, NSUInteger kind, F32 title_height);
internal void os_mac_gfx_set_window_buttons_position(OS_MAC_Window* window);

//- rjf: event helpers
internal OS_Event *   os_mac_gfx_event_list_push_key(Arena* arena, OS_EventList *evts, OS_MAC_Window* os_window,  OS_Key key, B32 is_down);
internal B32          os_mac_gfx_next_event(Arena * arena, B32 wait, OS_EventList* evts);
internal OS_EventList os_mac_gfx_dequeue_events(Arena * arena);
internal void         os_mac_gfx_send_dummy_event(void);

//- rjf: window delegate methods
internal void       os_mac_gfx_did_resize_handler(id v, SEL s, id notification);
internal BOOL       os_mac_gfx_should_close_handler(id v, SEL s, id window);
internal NSUInteger os_mac_gfx_dragging_entered_handler(id v, SEL s, id sender);
internal BOOL       os_mac_gfx_perform_drag_handler(id v, SEL s, id sender);

#endif // OS_GFX_MAC_H
