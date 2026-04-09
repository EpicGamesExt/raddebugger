////////////////////////////////
//~ rjf: @os_hooks Helpers

#include "os_gfx_mac.h"
#include <ctype.h>
#include <mach-o/dyld.h>
#include <objc/runtime.h>

#include "os_gfx_mac_keycode.h"

////////////////////////////////
//~ rjf: @os_hooks Helpers

// AppKit enum defininitions

enum {
  NSWindowStyleMaskBorderless             = 0,
  NSWindowStyleMaskTitled                 = 1 << 0,
  NSWindowStyleMaskClosable               = 1 << 1,
  NSWindowStyleMaskMiniaturizable         = 1 << 2,
  NSWindowStyleMaskResizable              = 1 << 3,
  NSWindowStyleMaskUnifiedTitleAndToolbar = 1 << 12,
  NSWindowStyleMaskFullScreen             = 1 << 14,
  NSWindowStyleMaskFullSizeContentView    = 1 << 15,
  NSWindowStyleMaskUtilityWindow          = 1 << 4,
  NSWindowStyleMaskDocModalWindow         = 1 << 6,
  NSWindowStyleMaskNonactivatingPanel     = 1 << 7,
  NSWindowStyleMaskHUDWindow              = 1 << 13
};

enum {
  NSEventTypeLeftMouseDown                = 1,
  NSEventTypeLeftMouseUp                  = 2,
  NSEventTypeRightMouseDown               = 3,
  NSEventTypeRightMouseUp                 = 4,
  NSEventTypeMouseMoved                   = 5,
  NSEventTypeLeftMouseDragged             = 6,
  NSEventTypeRightMouseDragged            = 7,
  NSEventTypeMouseEntered                 = 8,
  NSEventTypeMouseExited                  = 9,
  NSEventTypeKeyDown                      = 10,
  NSEventTypeKeyUp                        = 11,
  NSEventTypeFlagsChanged                 = 12,
  NSEventTypeAppKitDefined                = 13,
  NSEventTypeSystemDefined                = 14,
  NSEventTypeApplicationDefined           = 15,
  NSEventTypePeriodic                     = 16,
  NSEventTypeCursorUpdate                 = 17,
  NSEventTypeScrollWheel                  = 22,
  NSEventTypeTabletPoint                  = 23,
  NSEventTypeTabletProximity              = 24,
  NSEventTypeOtherMouseDown               = 25,
  NSEventTypeOtherMouseUp                 = 26,
  NSEventTypeOtherMouseDragged            = 27,
  // Introduced in macOS on 10.5.2 and later
  NSEventTypeGesture                      = 29,
  NSEventTypeMagnify                      = 30,
  NSEventTypeSwipe                        = 31,
  NSEventTypeRotate                       = 18,
  NSEventTypeBeginGesture                 = 19,
  NSEventTypeEndGesture                   = 20,
  
  NSEventTypeSmartMagnify                 = 32,
  NSEventTypeQuickLook                    = 33,
  
  NSEventTypePressure                     = 34,
  NSEventTypeDirectTouch                  = 37,
  // Introduced in macOS Tahoe
  NSEventTypeChangeMode                   = 38,
};

typedef NSUInteger NSEventModifierFlags;
enum {
    NSEventModifierFlagCapsLock           = 1 << 16, // Set if Caps Lock key is pressed.
    NSEventModifierFlagShift              = 1 << 17, // Set if Shift key is pressed.
    NSEventModifierFlagControl            = 1 << 18, // Set if Control key is pressed.
    NSEventModifierFlagOption             = 1 << 19, // Set if Option or Alternate key is pressed.
    NSEventModifierFlagCommand            = 1 << 20, // Set if Command key is pressed.
    NSEventModifierFlagNumericPad         = 1 << 21, // Set if any key in the numeric keypad is pressed.
    NSEventModifierFlagHelp               = 1 << 22, // Set if the Help key is pressed.
    NSEventModifierFlagFunction           = 1 << 23, // Set if any function key is pressed.
};

typedef NSUInteger NSWindowButton;
enum {
  NSWindowCloseButton,
  NSWindowMiniaturizeButton,
  NSWindowZoomButton,
};

extern id const NSDefaultRunLoopMode;

extern id const NSImageNameCaution;
extern id const NSImageNameInfo;

id
NSString_fromUTF8(String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  String8 string_copy = push_str8_copy(scratch.arena, string);
  id result = msg1(id, cls("NSString"), "stringWithUTF8String:", const char*, string_copy.str);
  scratch_end(scratch);
  return result;
}

internal void
os_mac_gfx_event_list_push_key(Arena* arena, OS_EventList *evts, OS_MAC_Window* os_window,  OS_Key key, B32 is_down)
{
  OS_EventKind kind = is_down ? OS_EventKind_Press : OS_EventKind_Release;
  if(is_down)
  {
    os_mac_gfx_state->keys[key] |= OS_MAC_KeyState_Down;
  }
  else
  {
    os_mac_gfx_state->keys[key] &= ~OS_MAC_KeyState_Down;
  }
  OS_Event *e = os_event_list_push_new(arena, evts, kind);
  e->window.u64[0] = (U64)os_window;
  e->key = key;
  e->modifiers = os_mac_gfx_state->modifiers;
  e->pos = os_mouse_from_window(e->window);
}

internal void
os_gfx_max_move_window_button(id window, NSWindowButton kind, F32 title_height) {
  id close_button = msg1(id, window, "standardWindowButton:", NSWindowButton, kind);

  CGRect frame = msg(CGRect, close_button, "frame");
  CGPoint new_origin = frame.origin;
  
  F32 size = frame.size.width;

  F32 base_offset_x = (size + 6.0) * kind;
  F32 base_offset_y = size + 5.2;
  F32 offset = 0.18 * title_height;

  new_origin.x = base_offset_x + offset;
  new_origin.y = base_offset_y - offset;
  
  msg1(void, close_button, "setFrameOrigin:", CGPoint, new_origin);
}

internal void
os_gfx_mac_set_window_buttons_position(OS_MAC_Window* window) {
  F32 title_height = window->custom_border_title_thickness;
 
  os_gfx_max_move_window_button(window->win, NSWindowCloseButton, title_height);
  os_gfx_max_move_window_button(window->win, NSWindowMiniaturizeButton, title_height);
  os_gfx_max_move_window_button(window->win, NSWindowZoomButton, title_height);
}

internal B32
os_mac_gfx_next_event(Arena * arena, B32 wait, OS_EventList* evts)
{
  B32 propagate = 1;

  id app = msg(id, cls("NSApplication"), "sharedApplication");
  NSInteger matchAll = -1;
  id data = msg(id, cls("NSDate"), wait ? "distantFuture" : "distantPast");
  id event = msg4(
    id, app, "nextEventMatchingMask:untilDate:inMode:dequeue:", 
    NSInteger, matchAll,
    id, data,
    id, NSDefaultRunLoopMode,
    BOOL, YES
  );

  if(event == 0)
  {
    return 0;
  }

  NSInteger type = msg(NSInteger, event, "type");
  id window = msg(id, event, "window");

  OS_MAC_Window* os_window = 0;
  if(window != 0)
  {
    os_window = (OS_MAC_Window *)objc_getAssociatedObject(window, "rad_window");
  }

  switch(type)
  {
    default: break;

    case NSEventTypeLeftMouseDown:
    case NSEventTypeRightMouseDown:
    case NSEventTypeOtherMouseDown:
    case NSEventTypeLeftMouseUp:
    case NSEventTypeRightMouseUp:
    case NSEventTypeOtherMouseUp:
      {
        U32 button_number = msg(U32, event, "buttonNumber");
        B32 is_down = type == NSEventTypeLeftMouseDown ||
                      type == NSEventTypeLeftMouseDown ||
                      type == NSEventTypeLeftMouseDown;

        OS_Key key = OS_Key_Null;
        switch(button_number)
        {
          case 0: {key = OS_Key_LeftMouseButton;}break;
          case 1: {key = OS_Key_RightMouseButton;}break;
          case 2: {key = OS_Key_MiddleMouseButton;}break;
        }

        os_mac_gfx_event_list_push_key(arena, evts, os_window, key, is_down);
      }
      break;
  
    // Key Events
    case NSEventTypeKeyDown:
    case NSEventTypeKeyUp:
      {
        propagate = 0;

        unsigned short ns_key_code = msg(unsigned short, event, "keyCode");
        OS_Key key = os_mac_gfx_keycode_table[ns_key_code];
        B32 is_down = type == NSEventTypeKeyDown;
        
        os_mac_gfx_event_list_push_key(arena, evts, os_window, key, is_down);

        NSEventModifierFlags flags = msg(NSEventModifierFlags, event, "modifierFlags");
        B32 skip_char = flags & (NSEventModifierFlagControl | NSEventModifierFlagCommand);

        if(is_down && !skip_char)
        {
          id ns_characters = msg(id, event, "characters");
          char* c_characters = msg(char *, ns_characters, "UTF8String");
          U32 character = c_characters[0];
          if(character >= 32 && character < 127)
          {
            OS_Event *event = os_event_list_push_new(arena, evts, OS_EventKind_Text);
            event->window.u64[0] = (U64)os_window;
            event->modifiers = os_mac_gfx_state->modifiers;
            event->character = character;
          }
        }
      }
      break;

    case NSEventTypeFlagsChanged:
    {
      unsigned short ns_key_code = msg(unsigned short, event, "keyCode");
      OS_Key key = os_mac_gfx_keycode_table[ns_key_code];
      B32 is_down = !os_key_is_down(key);

      os_mac_gfx_event_list_push_key(arena, evts, os_window, key, is_down);

      NSEventModifierFlags flags = msg(NSEventModifierFlags, event, "modifierFlags");

      OS_Modifiers modifiers = 0;

      if(flags & (NSEventModifierFlagControl | NSEventModifierFlagCommand))
      {
        modifiers |= OS_Modifier_Ctrl;
      }
      if(flags & NSEventModifierFlagShift)
      {
        modifiers |= OS_Modifier_Shift;
      }
      if(flags & NSEventModifierFlagOption)
      {
        modifiers |= OS_Modifier_Alt;
      }

      os_mac_gfx_state->modifiers = modifiers;
    }
    break;

    case NSEventTypeScrollWheel:
    {
      F64 delta_y = msg(F64, event, "scrollingDeltaY");

      OS_Event *e = os_event_list_push_new(arena, evts, OS_EventKind_Scroll);
      e->window.u64[0] = (U64)os_window;
      e->delta = v2f32(0, -delta_y);
      e->pos = os_mouse_from_window(e->window);
    }
    break;

    case NSEventTypeLeftMouseDragged:
    {
      OS_Handle window_handle = {(U64)os_window};
      Vec2F32 pos_client = os_mouse_from_window(window_handle);

      if(pos_client.y < os_window->custom_border_title_thickness)
      {
        B32 is_over_title_bar_client_area = 0;

        //- yuraiz: check against title bar client areas
        for(OS_MAC_TitleBarClientArea *area = os_window->first_title_bar_client_area;
          area != 0;
          area = area->next)
        {
          Rng2F32 rect = area->rect;
          if(rect.x0 <= pos_client.x && pos_client.x < rect.x1 &&
            rect.y0 <= pos_client.y && pos_client.y < rect.y1)
            {
              is_over_title_bar_client_area = 1;
              break;
            }
        }
        if(!is_over_title_bar_client_area)
        {
          msg1(void, window, "performWindowDragWithEvent:", id, event);
        }
      }

    }
    break;
  }

  if(propagate)
  {
    msg1(void, app, "sendEvent:", id, event);
  }

  return 1;
}

static void
os_gfx_mac_did_resize_handler(id v, SEL s, id notification)
{
  id window = msg(id, notification, "object");
  OS_MAC_Window* os_window = (OS_MAC_Window *)objc_getAssociatedObject(window, "rad_window");
  os_gfx_mac_set_window_buttons_position(os_window);

  rd_frame();

  msg1(void, window, "setViewsNeedDisplay:", BOOL, YES);
}

static BOOL os_gfx_mac_should_close_handler(id v, SEL s, id w)
{
  id app = msg(id, cls("NSApplication"), "sharedApplication");
  NSUInteger window_count = msg(NSUInteger, msg(id, app, "windows"), "count");

  if(window_count < 2)
  {
    msg1(void, app, "terminate:", id, app);
  }

  return YES;
}

////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

#include <CoreFoundation/CFCGTypes.h>
#include <objc/NSObjCRuntime.h>
#include <stdio.h>
internal void
os_gfx_init(void)
{  
  Arena *arena = arena_alloc();
  os_mac_gfx_state = push_array(arena, OS_MAC_GfxState, 1);
  os_mac_gfx_state->arena = arena;

  //- yuraiz: fill out gfx info
  os_mac_gfx_state->gfx_info.double_click_time = 0.5f;
  os_mac_gfx_state->gfx_info.caret_blink_time = 0.5f;
  os_mac_gfx_state->gfx_info.default_refresh_rate = 60.f;

  //- yuraiz: setup NSApplication
  id app = msg(id, cls("NSApplication"), "sharedApplication");
  msg1(void, app, "setActivationPolicy:", NSInteger, 0);

  //- yuraiz: add minimal menu
  // https://hero.handmade.network/forums/code-discussion/t/1409-main_game_loop_on_os_x

  id menubar = msg(id, msg(id, cls("NSMenu"), "new"), "autorelease");
  id app_menu_item = msg(id, msg(id, cls("NSMenuItem"), "new"), "autorelease");
  msg1(void, menubar, "addItem:", id, app_menu_item);
  msg1(void, app, "setMainMenu:", id, menubar);

  // Then we add the quit item to the menu. Fortunately the action is simple since terminate: is
  // already implemented in NSApplication and the NSApplication is always in the responder chain.
  id app_menu = msg(id, msg(id, cls("NSMenu"), "new"), "autorelease");
  id appName = msg(id, msg(id, cls("NSProcessInfo"), "processInfo"), "processName");
  id quitString = NSString_fromUTF8(str8_lit("Quit "));
  id quitTitle = msg1(id, quitString, "stringByAppendingString:", id, appName);

  id quitMenuItem = msg(id, 
    msg3(id, 
      msg(id, cls("NSMenuItem"), "alloc"),
      "initWithTitle:action:keyEquivalent:",
      id, quitTitle, SEL, sel_getUid("terminate:"), id, NSString_fromUTF8(str8_lit("q"))
    ),
    "autorelease"
  );

  msg1(void, app_menu, "addItem:", id, quitMenuItem);
  msg1(void, app_menu_item, "setSubmenu:", id, app_menu);

  msg1(void, app, "activateIgnoringOtherApps:", BOOL, YES);

  //- yuraiz: create window delegate class
  Class win_delegate = objc_allocateClassPair((Class)cls("NSObject"), "RADWinDelegate", 0);

  class_addMethod(win_delegate, sel_getUid("windowDidResize:"),
    (IMP)os_gfx_mac_did_resize_handler, "v@:@");

  class_addMethod(win_delegate, sel_getUid("windowShouldClose:"),
    (IMP)os_gfx_mac_should_close_handler, "v@:@");

  objc_registerClassPair(win_delegate);
}

////////////////////////////////
//~ rjf: @os_hooks Graphics System Info (Implemented Per-OS)

internal OS_GfxInfo *
os_get_gfx_info(void)
{
  return &os_mac_gfx_state->gfx_info;
}

////////////////////////////////
//~ rjf: @os_hooks Clipboards (Implemented Per-OS)

internal void
os_set_clipboard_text(String8 string)
{
}

internal String8
os_get_clipboard_text(Arena *arena)
{
  return str8_zero();
}

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)


internal OS_Handle
os_window_open(Rng2F32 rect, OS_WindowFlags flags, String8 title)
{
  Vec2F32 resolution = dim_2f32(rect);

  CGRect contentRect = {{0, 0}, {resolution.x, resolution.y}};
  NSInteger styleMask = NSWindowStyleMaskResizable |
                        NSWindowStyleMaskClosable |
                        NSWindowStyleMaskMiniaturizable |
                        NSWindowStyleMaskFullSizeContentView |
                        NSWindowStyleMaskTitled;
  NSInteger backingStore = 2;
  id window = msg4(id, msg(id, cls("NSWindow"), "alloc"),
                "initWithContentRect:styleMask:backing:defer:",
                CGRect, contentRect,
                NSUInteger, styleMask,
                NSUInteger, backingStore,
                BOOL, NO
              );

  if(window == 0)
  {
    fprintf(stderr, "Window is nil");
    os_abort(1);
  }

  msg1(void, window, "setTitle:", id,  NSString_fromUTF8(title));
  msg1(void, window, "makeKeyAndOrderFront:", id, NULL);
  msg1(void, window, "setTitlebarAppearsTransparent:", BOOL, YES);
  msg1(void, window, "setTitleVisibility:", NSUInteger, 1);
  msg1(void, window, "setMovable:", BOOL, NO);
  msg(void, window, "center");

  id win_delegate = msg(id, msg(id, cls("RADWinDelegate"), "alloc"), "init");
  msg1(void, window, "setDelegate:", id, win_delegate);

  OS_MAC_Window* os_window = push_array(os_mac_gfx_state->arena, OS_MAC_Window, 1);
  os_window->win = window;
  os_window->custom_border = 1;
  os_window->paint_arena = arena_alloc();
  objc_setAssociatedObject(window, "rad_window", (id)os_window, OBJC_ASSOCIATION_ASSIGN);

  os_gfx_mac_set_window_buttons_position(os_window);

  OS_Handle handle = {(U64)os_window};
  return handle;
}

internal void
os_window_close(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_MAC_Window *w = (OS_MAC_Window *)handle.u64[0];
  msg1(void, w->win, "performClose:", id, NULL);
}

internal void
os_window_set_title(OS_Handle handle, String8 title)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_MAC_Window *w = (OS_MAC_Window *)handle.u64[0];
  msg1(void, w->win, "setTitle:", id,  NSString_fromUTF8(title));
}

internal void
os_window_first_paint(OS_Handle handle)
{
// dummy
}

internal void
os_window_focus(OS_Handle window)
{
// dummy
}

internal B32
os_window_is_focused(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_MAC_Window *w = (OS_MAC_Window *)handle.u64[0];
  B32 result = msg(BOOL, w->win, "isKeyWindow");
  return result;
}

internal B32
os_window_is_fullscreen(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_MAC_Window *w = (OS_MAC_Window *)handle.u64[0];
  NSInteger styleMask = msg(NSInteger, w->win, "styleMask");
  return styleMask & NSWindowStyleMaskFullScreen;
}

internal void
os_window_set_fullscreen(OS_Handle handle, B32 fullscreen)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_MAC_Window *w = (OS_MAC_Window *)handle.u64[0];
  B32 already_fullscreen = os_window_is_fullscreen(handle);

  if(fullscreen != already_fullscreen)
  {
    msg1(void, w->win, "toggleFullScreen:", id, NULL);
  }
}

internal B32
os_window_is_maximized(OS_Handle window)
{
  return os_window_is_fullscreen(window);
}

internal void
os_window_set_maximized(OS_Handle window, B32 maximized)
{
  return os_window_set_fullscreen(window, maximized);
}

internal B32
os_window_is_minimized(OS_Handle window)
{
  if(os_handle_match(window, os_handle_zero())) {return 0;}
  OS_MAC_Window *w = (OS_MAC_Window *)window.u64[0];
  BOOL result = msg(BOOL, w->win, "isMiniaturized");
  return result;
}

internal void
os_window_set_minimized(OS_Handle window, B32 minimized)
{
  if(os_handle_match(window, os_handle_zero())) {return;}
  OS_MAC_Window *w = (OS_MAC_Window *)window.u64[0];
  msg1(void, w->win, minimized ? "miniaturize:" : "deminiaturize:", id, NULL);
}

internal void
os_window_bring_to_front(OS_Handle window)
{
  if(os_handle_match(window, os_handle_zero())) {return;}
  OS_MAC_Window *w = (OS_MAC_Window *)window.u64[0];
  msg1(void, w->win, "makeKeyAndOrderFront:", id, NULL);
}

internal void
os_window_set_monitor(OS_Handle window, OS_Handle monitor)
{
}

internal void
os_window_clear_custom_border_data(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_MAC_Window *window = (OS_MAC_Window *)handle.u64[0];
  if(window->custom_border)
  {
    arena_clear(window->paint_arena);
    window->first_title_bar_client_area = window->last_title_bar_client_area = 0;
  }
}

internal void
os_window_push_custom_title_bar(OS_Handle handle, F32 thickness)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_MAC_Window *window = (OS_MAC_Window *)handle.u64[0];
  if(window->custom_border_title_thickness != thickness)
  {
    window->custom_border_title_thickness = thickness;
    os_gfx_mac_set_window_buttons_position(window);
  }
}

internal void
os_window_push_custom_edges(OS_Handle handle, F32 thickness)
{
}

internal void
os_window_push_custom_title_bar_client_area(OS_Handle handle, Rng2F32 rect)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_MAC_Window *window = (OS_MAC_Window *)handle.u64[0];
  if(window->custom_border)
  {
    OS_MAC_TitleBarClientArea *area = push_array(window->paint_arena, OS_MAC_TitleBarClientArea, 1);
    if(area != 0)
    {
      area->rect = rect;
      SLLQueuePush(window->first_title_bar_client_area, window->last_title_bar_client_area, area);
    }
  }
}

internal Rng2F32
os_rect_from_window(OS_Handle window)
{
  if(os_handle_match(window, os_handle_zero())) {return;}
  OS_MAC_Window *w = (OS_MAC_Window *)window.u64[0];
  
  id content_view = msg(id, w->win, "contentView");
  CGRect rect = msg(CGRect, content_view, "frame");

  Rng2F32 result = r2f32(v2f32(0, 0), v2f32(rect.size.width, rect.size.height));
  return result;
}

internal Rng2F32
os_client_rect_from_window(OS_Handle window)
{
  if(os_handle_match(window, os_handle_zero())) {return;}
  OS_MAC_Window *w = (OS_MAC_Window *)window.u64[0];
  
  id content_view = msg(id, w->win, "contentView");
  CGRect rect = msg(CGRect, content_view, "frame");
  CGRect scaled_rect = msg1(CGRect, content_view, "convertRectToBacking:", CGRect, rect);

  Rng2F32 result = r2f32(v2f32(0, 0), v2f32(scaled_rect.size.width, scaled_rect.size.height));
  return result;
}

internal F32
os_dpi_from_window(OS_Handle window)
{
  return 96.f;
}

////////////////////////////////
//~ rjf: @os_hooks External Windows (Implemented Per-OS)

internal OS_Handle
os_focused_external_window(void)
{
  OS_Handle result = {0};
  // TODO(rjf)
  return result;
}

internal void
os_focus_external_window(OS_Handle handle)
{
  // TODO(rjf)
}

////////////////////////////////
//~ rjf: @os_hooks Monitors (Implemented Per-OS)

internal OS_HandleArray
os_push_monitors_array(Arena *arena)
{
  OS_HandleArray arr = {0};
  return arr;
}

internal OS_Handle
os_primary_monitor(void)
{
  OS_Handle handle = {1};
  return handle;
}

internal OS_Handle
os_monitor_from_window(OS_Handle window)
{
  OS_Handle handle = {1};
  return handle;
}

internal String8
os_name_from_monitor(Arena *arena, OS_Handle monitor)
{
  return str8_zero();
}

internal Vec2F32
os_dim_from_monitor(OS_Handle monitor)
{
  Vec2F32 v = v2f32(1000, 1000);
  return v;
}

internal F32
os_dpi_from_monitor(OS_Handle monitor)
{
  return 96.f;
}

////////////////////////////////
//~ rjf: @os_hooks Events (Implemented Per-OS)

internal void
os_send_wakeup_event(void)
{
}

internal OS_EventList
os_get_events(Arena *arena, B32 wait)
{
  OS_EventList evts = {0};

  if(wait)
  {
    os_mac_gfx_next_event(arena, wait, &evts);
  }

  while(os_mac_gfx_next_event(arena, 0, &evts)) {}

  return evts;
}

internal OS_Modifiers
os_get_modifiers(void)
{
  return os_mac_gfx_state->modifiers;
}

internal B32
os_key_is_down(OS_Key key)
{
  B32 down = 0;
  if(os_mac_gfx_state->keys[key] & OS_MAC_KeyState_Down)
  {
    down = 1;
  }
  return down;
}

internal Vec2F32
os_mouse_from_window(OS_Handle window)
{
  if(os_handle_match(window, os_handle_zero())) {return v2f32(0, 0);}
  OS_MAC_Window *w = (OS_MAC_Window *)window.u64[0];

  CGPoint mouse_loc = msg(CGPoint, w->win, "mouseLocationOutsideOfEventStream");

  id content_view = msg(id, w->win, "contentView");
  CGRect rect = msg(CGRect, content_view, "frame");
  mouse_loc.y = rect.size.height - mouse_loc.y;

  F64 scale = msg(F64, w->win, "backingScaleFactor");

  return v2f32(mouse_loc.x * scale, mouse_loc.y * scale);
}

////////////////////////////////
//~ rjf: @os_hooks Cursors (Implemented Per-OS)

internal void
os_set_cursor(OS_Cursor cursor)
{
  NSInteger NSCursorBottomRight = 12;
  NSInteger NSCursorTopRight = 9;

  id ns_cursor = os_mac_gfx_state->cursors[cursor];
  if(ns_cursor == 0)
  {
    switch(cursor)
    {
      case OS_Cursor_Pointer:
        ns_cursor = msg(id, cls("NSCursor"), "arrowCursor");
        break;
      case OS_Cursor_IBar:
        ns_cursor = msg(id, cls("NSCursor"), "IBeamCursor");
        break;
      case OS_Cursor_LeftRight:
        ns_cursor = msg(id, cls("NSCursor"), "resizeLeftRightCursor");
        break;
      case OS_Cursor_UpDown:
        ns_cursor = msg(id, cls("NSCursor"), "resizeUpDownCursor");
        break;
      case OS_Cursor_DownRight:
        ns_cursor = msg2(id, cls("NSCursor"), "frameResizeCursorFromPosition:inDirections:",
          NSInteger, NSCursorBottomRight,
          NSInteger, 2
        );
        break;
      case OS_Cursor_UpRight:
        ns_cursor = msg2(id, cls("NSCursor"), "frameResizeCursorFromPosition:inDirections:",
          NSInteger, NSCursorTopRight,
          NSInteger, 2
        );
        break;
      case OS_Cursor_UpDownLeftRight:
        ns_cursor = msg(id, cls("NSCursor"), "openHandCursor");
        break;
      case OS_Cursor_HandPoint:
        ns_cursor = msg(id, cls("NSCursor"), "openHandCursor");
        break;
      case OS_Cursor_Disabled:
        ns_cursor = msg(id, cls("NSCursor"), "operationNotAllowedCursor");
        break;
    }
    os_mac_gfx_state->cursors[cursor] = ns_cursor;
  }

  msg(void, ns_cursor, "set");
}

////////////////////////////////
//~ rjf: @os_hooks Native User-Facing Graphical Messages (Implemented Per-OS)

internal void
os_graphical_message(B32 error, String8 title, String8 message)
{
  // NOTE(yurai): The funciton can be run without app initialization,
  // so ensure "sharedApplication" is created and configured.
  id app = msg(id, cls("NSApplication"), "sharedApplication");
  msg1(void, app, "setActivationPolicy:", NSInteger, 0);

  id alert = msg(id, msg(id, cls("NSAlert"), "alloc"), "init");

  id icon_name = error ? NSImageNameCaution : NSImageNameInfo;
  id icon = msg1(id, cls("NSImage"), "imageNamed:", id, icon_name);
  
  msg1(void, alert, "setMessageText:", id, NSString_fromUTF8(title));
  msg1(void, alert, "setInformativeText:", id, NSString_fromUTF8(message));
  msg1(void, alert, "setIcon:", id, icon);

  msg1(void, msg(id, alert, "window"), "makeKeyAndOrderFront:", id, NULL);
  msg(void, alert, "runModal");
}

internal String8
os_graphical_pick_file(Arena *arena, String8 initial_path)
{
  id dir_path = NSString_fromUTF8(initial_path);
  id dir_url = msg1(id, cls("NSURL"), "fileURLWithPath:", id, dir_path);

  id panel = msg(id, cls("NSOpenPanel"), "openPanel");

  msg1(void, panel, "setDirectoryURL:", id, dir_url);
  msg1(void, panel, "setCanChooseFiles:", BOOL, YES);
  msg1(void, panel, "setCanChooseDirectories:", BOOL, NO);
  msg1(void, panel, "setAllowsMultipleSelection:", BOOL, NO);

  NSInteger response = msg(NSInteger, panel, "runModal");
  if(response == 1)
  {
    id urls = msg(id, panel, "URLs");
    id url = msg(id, urls, "firstObject");
    id path = msg(id, url, "path");
    char* cstring = msg(char *, path, "UTF8String");
    String8 result = str8_cstring(cstring);
    return result;
  }

  return str8_zero();
}

////////////////////////////////
//~ rjf: @os_hooks Shell Operations

internal void
os_show_in_filesystem_ui(String8 path)
{
  Temp scratch = scratch_begin(0, 0);

  String8 path_arg = str8_copy(scratch.arena, path);

  char* args[] = {
    "--reveal",
    path_arg.str,
    0,
  };

  execve("/usr/bin/open", args, 0);

  scratch_end(scratch);
}

internal void
os_open_in_browser(String8 url)
{
  Temp scratch = scratch_begin(0, 0);

  String8 url_arg = str8_copy(scratch.arena, url);

  char* args[] = {
    "--url",
    url_arg.str,
    0,
  };

  execve("/usr/bin/open", args, 0);

  scratch_end(scratch);
}
