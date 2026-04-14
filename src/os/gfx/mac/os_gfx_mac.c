////////////////////////////////
//~ rjf: @os_hooks Helpers

internal id
NSString_fromUTF8(String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  String8 string_copy = push_str8_copy(scratch.arena, string);
  id result = msg1(id, cls("NSString"), "stringWithUTF8String:", const char*, (const char*)string_copy.str);
  scratch_end(scratch);
  return result;
}

internal void
os_gfx_max_move_window_button(id window, NSWindowButton kind, F32 title_height)
{
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
os_mac_gfx_set_window_buttons_position(OS_MAC_Window* window)
{
  F32 title_height = window->custom_border_title_thickness;
 
  os_gfx_max_move_window_button(window->win, NSWindowCloseButton, title_height);
  os_gfx_max_move_window_button(window->win, NSWindowMiniaturizeButton, title_height);
  os_gfx_max_move_window_button(window->win, NSWindowZoomButton, title_height);
}

internal OS_Event *
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
  OS_Event* e = os_event_list_push_new(arena, evts, kind);
  e->window.u64[0] = (U64)os_window;
  e->key = key;
  e->modifiers = os_mac_gfx_state->modifiers;
  e->pos = os_mouse_from_window(e->window);
  return e;
}

internal B32
os_mac_gfx_next_event(Arena * arena, B32 wait, OS_EventList* evts)
{
  B32 propagate = 1;

  id app = msg(id, cls("NSApplication"), "sharedApplication");
  NSInteger matchAll = -1;
  id date = msg(id, cls("NSDate"), wait ? "distantFuture" : "distantPast");
  id event = msg4(
    id, app, "nextEventMatchingMask:untilDate:inMode:dequeue:", 
    NSInteger, matchAll,
    id, date,
    id, NSDefaultRunLoopMode,
    BOOL, YES
  );
  msg(void, date, "release");

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

        OS_Event *event = os_mac_gfx_event_list_push_key(arena, evts, os_window, key, is_down);

        if(os_window == 0){break;}

        //- yuraiz: check if dragging the window
        os_window->dragging_window = 0;
        if(key == OS_Key_LeftMouseButton && is_down)
        {
          Vec2F32 pos_client = event->pos;
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
            os_window->dragging_window = !is_over_title_bar_client_area;
          }
        }
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
          msg(void, ns_characters, "release");
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
      if(os_window != 0 && os_window->dragging_window)
      {
        msg1(void, window, "performWindowDragWithEvent:", id, event);
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

internal OS_EventList
os_mac_gfx_dequeue_events(Arena * arena)
{
  OS_EventList result = {0};
  if(os_mac_gfx_state->event_queue.count > 0)
  {
    result = os_event_list_copy(arena, &os_mac_gfx_state->event_queue);
    OS_EventList empty_list = {0};
    os_mac_gfx_state->event_queue = empty_list;
    arena_clear(os_mac_gfx_state->event_arena);
  }
  return result;
}

internal void
os_mac_gfx_send_dummy_event(void)
{
  CGPoint location = {0, 0};
  // NOTE(yuraiz): That's just a dummy app-defined NSEvent
  SEL init_event = sel_getUid("otherEventWithType:location:modifierFlags:timestamp:windowNumber:context:subtype:data1:data2:");
  id dummy = ((id (*)(id, SEL, NSInteger, CGPoint, NSUInteger, double, NSInteger, id, short, NSInteger, NSInteger))objc_msgSend)(
    cls("NSEvent"), init_event, NSEventTypeApplicationDefined, location, 0, 0, 0, 0, 0, 0, 0
  );
  id app = msg(id, cls("NSApplication"), "sharedApplication");
  msg2(id, app, "postEvent:atStart:", id, dummy, BOOL, YES);
}

internal void
os_mac_gfx_did_resize_handler(id v, SEL s, id notification)
{
  id window = msg(id, notification, "object");
  OS_MAC_Window* os_window = (OS_MAC_Window *)objc_getAssociatedObject(window, "rad_window");
  os_mac_gfx_set_window_buttons_position(os_window);

  rd_frame();

  msg1(void, window, "setViewsNeedDisplay:", BOOL, YES);
}

internal BOOL
os_mac_gfx_should_close_handler(id v, SEL s, id window)
{
  OS_MAC_Window* os_window = (OS_MAC_Window *)objc_getAssociatedObject(window, "rad_window");
  DLLRemove(os_mac_gfx_state->first_window, os_mac_gfx_state->last_window, os_window);
  SLLStackPush(os_mac_gfx_state->free_window, os_window);

  id app = msg(id, cls("NSApplication"), "sharedApplication");
  NSUInteger window_count = msg(NSUInteger, msg(id, app, "windows"), "count");

  if(window_count < 2)
  {
    os_event_list_push_new(
      os_mac_gfx_state->event_arena,
      &os_mac_gfx_state->event_queue,
      OS_EventKind_WindowClose
    );
    os_mac_gfx_send_dummy_event();
  }

  return YES;
}

internal NSUInteger
os_mac_gfx_dragging_entered_handler(id v, SEL s, id sender)
{
  // NOTE(yuraiz): The window accepts only file dragging. So just permit the copy
  // https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/DragandDrop/Tasks/acceptingdrags.html
  // NSDragOperationCopy
  return 1;
}

internal BOOL
os_mac_gfx_perform_drag_handler(id v, SEL s, id sender)
{
  id pboard = msg(id, sender, "draggingPasteboard");
  id window = msg(id, sender, "draggingDestinationWindow");

  OS_MAC_Window* os_window = (OS_MAC_Window *)objc_getAssociatedObject(window, "rad_window");

  id types = msg(id, pboard, "types");

  if (msg1(BOOL, types, "containsObject:", id, NSPasteboardTypeFileURL))
  {
    id classes = msg1(id, cls("NSArray"), "arrayWithObject:", id, cls("NSURL"));
    
    id options = msg2(
      id, cls("NSDictionary"), 
      "dictionaryWithObject:forKey:", 
      id, msg1(id, cls("NSNumber"), "numberWithBool:", BOOL, YES),
      id, NSPasteboardURLReadingFileURLsOnlyKey
    );
    
    id fileURLs = msg2(id, pboard, "readObjectsForClasses:options:", 
                      id, classes, id, options);
                      
    NSUInteger count = msg(NSUInteger, fileURLs, "count");

    Arena *event_arena = os_mac_gfx_state->event_arena;

    if(count > 0)
    {
      OS_Event* event = os_event_list_push_new(
        event_arena, 
        &os_mac_gfx_state->event_queue,
        OS_EventKind_FileDrop
      );
    
      event->window.u64[0] = (U64)os_window;
      event->pos = os_mouse_from_window(event->window);
      for EachIndex(i, count)
      {
        id url = msg1(id, fileURLs, "objectAtIndex:", NSUInteger, i);
        id path = msg(id, url, "path");
        char* cstring = msg(char *, path, "UTF8String");

        struct stat path_stat;
        stat(cstring, &path_stat);
        B32 is_dir = S_ISDIR(path_stat.st_mode);

        if(!is_dir)
        {
          String8 str8_path = str8_copy(event_arena, str8_cstring(cstring));
          str8_list_push(event_arena, &event->strings, str8_path);
        }
      }

      os_mac_gfx_send_dummy_event();
    }
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

  os_mac_gfx_state->first_window = 0;
  os_mac_gfx_state->last_window = 0;
  os_mac_gfx_state->free_window = 0;

  OS_EventList empty_list = {0};
  os_mac_gfx_state->event_queue = empty_list;
  os_mac_gfx_state->event_arena = arena_alloc();

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
    (IMP)os_mac_gfx_did_resize_handler, "v@:@");

  class_addMethod(win_delegate, sel_getUid("windowShouldClose:"),
    (IMP)os_mac_gfx_should_close_handler, "v@:@");

  class_addMethod(win_delegate, sel_getUid("draggingEntered:"),
    (IMP)os_mac_gfx_dragging_entered_handler, "v@:@");
  
  class_addMethod(win_delegate, sel_getUid("performDragOperation:"),
    (IMP)os_mac_gfx_perform_drag_handler, "v@:@");

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
  id ns_string =  NSString_fromUTF8(string);
  id pboard = msg(id, cls("NSPasteboard"), "generalPasteboard");
  msg(void, pboard, "clearContents");
  msg2(void, pboard, "setString:forType:", id, ns_string, id, NSPasteboardTypeString);
}

internal String8
os_get_clipboard_text(Arena *arena)
{
  id pboard = msg(id, cls("NSPasteboard"), "generalPasteboard");
  id ns_string = msg1(id, pboard, "stringForType:", id, NSPasteboardTypeString);

  char* cstring = msg(char *, ns_string, "UTF8String");
  String8 result = push_str8_copy(arena, str8_cstring(cstring));
  msg(void, ns_string, "release");

  return result;
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

  id pb_type_array = msg1(id, cls("NSArray"), "arrayWithObject:", id, NSPasteboardTypeFileURL);
  msg1(void, window, "registerForDraggedTypes:", id, pb_type_array);

  id win_delegate = msg(id, msg(id, cls("RADWinDelegate"), "alloc"), "init");
  msg1(void, window, "setDelegate:", id, win_delegate);

  OS_MAC_Window* os_window = os_mac_gfx_state->free_window;
  if(os_window)
  {
    os_window = SLLStackPop(os_mac_gfx_state->free_window);
  }
  else
  {
    os_window = push_array(os_mac_gfx_state->arena, OS_MAC_Window, 1);
  }
  MemoryZeroStruct(os_window);
  os_window->win = window;
  os_window->custom_border = 1;
  os_window->paint_arena = arena_alloc();
  objc_setAssociatedObject(window, "rad_window", (id)os_window, OBJC_ASSOCIATION_ASSIGN);

  os_mac_gfx_set_window_buttons_position(os_window);

  DLLPushBack(os_mac_gfx_state->first_window, os_mac_gfx_state->last_window, os_window);

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
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
  OS_MAC_Window *w = (OS_MAC_Window *)handle.u64[0];
  B32 result = msg(BOOL, w->win, "isKeyWindow");
  return result;
}

internal B32
os_window_is_fullscreen(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
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
    os_mac_gfx_set_window_buttons_position(window);
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
  if(os_handle_match(window, os_handle_zero())) {return r2f32(v2f32(0, 0), v2f32(0, 0));}
  OS_MAC_Window *w = (OS_MAC_Window *)window.u64[0];
  
  id content_view = msg(id, w->win, "contentView");
  CGRect rect = msg(CGRect, content_view, "frame");

  Rng2F32 result = r2f32(v2f32(0, 0), v2f32(rect.size.width, rect.size.height));
  return result;
}

internal Rng2F32
os_client_rect_from_window(OS_Handle window)
{
  if(os_handle_match(window, os_handle_zero())) {return r2f32(v2f32(0, 0), v2f32(0, 0));}
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
  OS_EventList evts = os_mac_gfx_dequeue_events(arena);

  if(wait && evts.count == 0)
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
      case OS_Cursor_COUNT: break;
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

  msg(void, alert, "layout");
  id alert_window = msg(id, alert, "window");
  msg(void, alert_window, "center");
  msg1(void, alert_window, "makeKeyAndOrderFront:", id, NULL);
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
    String8 result = push_str8_copy(arena, str8_cstring(cstring));
    msg(void, urls, "release");
    msg(void, path, "release");
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
    (char *)path_arg.str,
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
    (char *)url_arg.str,
    0,
  };

  execve("/usr/bin/open", args, 0);

  scratch_end(scratch);
}
