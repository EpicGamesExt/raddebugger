// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ dan: Helpers


internal OS_Key
os_lnx_keysym_to_oskey(KeySym keysym)
{
    OS_Key key = OS_Key_Null;
    switch(keysym)
    {
        default:
        {
            if(0){}
            else if(XK_F1 <= keysym && keysym <= XK_F24) { key = (OS_Key)(OS_Key_F1 + (keysym - XK_F1)); }
            else if('0' <= keysym && keysym <= '9')      { key = OS_Key_0 + (keysym-'0'); }
            else if(XK_KP_0 <= keysym && keysym <= XK_KP_9) { key = (OS_Key)(OS_Key_Num0 + (keysym - XK_KP_0)); }
        }break;
        case XK_Escape:{key = OS_Key_Esc;};break;
        case XK_minus:{key = OS_Key_Minus;}break;
        case XK_equal:{key = OS_Key_Equal;}break;
        case XK_bracketleft:{key = OS_Key_LeftBracket;}break;
        case XK_bracketright:{key = OS_Key_RightBracket;}break;
        case XK_semicolon:{key = OS_Key_Semicolon;}break;
        case XK_apostrophe:{key = OS_Key_Quote;}break;
        case XK_period:{key = OS_Key_Period;}break;
        case XK_comma:{key = OS_Key_Comma;}break;
        case XK_slash:{key = OS_Key_Slash;}break;
        case XK_backslash:{key = OS_Key_BackSlash;}break;
        case XK_grave:{key = OS_Key_Tick;}break; // Grave accent often maps to Tick
        case XK_a:case XK_A:{key = OS_Key_A;}break;
        case XK_b:case XK_B:{key = OS_Key_B;}break;
        case XK_c:case XK_C:{key = OS_Key_C;}break;
        case XK_d:case XK_D:{key = OS_Key_D;}break;
        case XK_e:case XK_E:{key = OS_Key_E;}break;
        case XK_f:case XK_F:{key = OS_Key_F;}break;
        case XK_g:case XK_G:{key = OS_Key_G;}break;
        case XK_h:case XK_H:{key = OS_Key_H;}break;
        case XK_i:case XK_I:{key = OS_Key_I;}break;
        case XK_j:case XK_J:{key = OS_Key_J;}break;
        case XK_k:case XK_K:{key = OS_Key_K;}break;
        case XK_l:case XK_L:{key = OS_Key_L;}break;
        case XK_m:case XK_M:{key = OS_Key_M;}break;
        case XK_n:case XK_N:{key = OS_Key_N;}break;
        case XK_o:case XK_O:{key = OS_Key_O;}break;
        case XK_p:case XK_P:{key = OS_Key_P;}break;
        case XK_q:case XK_Q:{key = OS_Key_Q;}break;
        case XK_r:case XK_R:{key = OS_Key_R;}break;
        case XK_s:case XK_S:{key = OS_Key_S;}break;
        case XK_t:case XK_T:{key = OS_Key_T;}break;
        case XK_u:case XK_U:{key = OS_Key_U;}break;
        case XK_v:case XK_V:{key = OS_Key_V;}break;
        case XK_w:case XK_W:{key = OS_Key_W;}break;
        case XK_x:case XK_X:{key = OS_Key_X;}break;
        case XK_y:case XK_Y:{key = OS_Key_Y;}break;
        case XK_z:case XK_Z:{key = OS_Key_Z;}break;
        case XK_space:{key = OS_Key_Space;}break;
        case XK_BackSpace:{key = OS_Key_Backspace;}break;
        case XK_Tab: case XK_ISO_Left_Tab: {key = OS_Key_Tab;}break; // Handle shift+tab
        case XK_Return: case XK_KP_Enter: {key = OS_Key_Return;}break;
        case XK_Shift_L: case XK_Shift_R: {key = OS_Key_Shift;}break;
        case XK_Control_L: case XK_Control_R: {key = OS_Key_Ctrl;}break;
        case XK_Alt_L: case XK_Alt_R: {key = OS_Key_Alt;}break;
        case XK_Menu: case XK_Super_L: case XK_Super_R: {key = OS_Key_Menu;}break; // Map Super/Windows key to Menu
        case XK_Scroll_Lock: {key = OS_Key_ScrollLock;}break;
        case XK_Pause: {key = OS_Key_Pause;}break;
        case XK_Insert: case XK_KP_Insert: {key = OS_Key_Insert;}break;
        case XK_Home: case XK_KP_Home: {key = OS_Key_Home;}break;
        case XK_Page_Up: case XK_KP_Page_Up: {key = OS_Key_PageUp;}break;
        case XK_Delete: case XK_KP_Delete: {key = OS_Key_Delete;}break;
        case XK_End: case XK_KP_End: {key = OS_Key_End;}break;
        case XK_Page_Down: case XK_KP_Page_Down: {key = OS_Key_PageDown;}break;
        case XK_Up: case XK_KP_Up: {key = OS_Key_Up;}break;
        case XK_Left: case XK_KP_Left: {key = OS_Key_Left;}break;
        case XK_Down: case XK_KP_Down: {key = OS_Key_Down;}break;
        case XK_Right: case XK_KP_Right: {key = OS_Key_Right;}break;
        case XK_Num_Lock: {key = OS_Key_NumLock;}break;
        case XK_KP_Divide: {key = OS_Key_NumSlash;}break;
        case XK_KP_Multiply: {key = OS_Key_NumStar;}break;
        case XK_KP_Subtract: {key = OS_Key_NumMinus;}break;
        case XK_KP_Add: {key = OS_Key_NumPlus;}break;
        case XK_KP_Decimal: {key = OS_Key_NumPeriod;}break;
        case XK_Caps_Lock: {key = OS_Key_CapsLock;} break;
    }
    return key;
}

internal OS_LNX_Window *
os_lnx_window_from_x11window(Window window)
{
  OS_LNX_Window *result = 0;
  for(OS_LNX_Window *w = os_lnx_gfx_state->first_window; w != 0; w = w->next)
  {
    if(w->window == window)
    {
      result = w;
      break;
    }
  }
  return result;
}

////////////////////////////////
//~ dan: @os_hooks Main Initialization API (Implemented Per-OS)

internal void
os_gfx_init(void)
{
  //- dan: initialize basics
  Arena *arena = arena_alloc();
  os_lnx_gfx_state = push_array(arena, OS_LNX_GfxState, 1);
  os_lnx_gfx_state->arena = arena;
  
  // Attempt to open a connection to the X server
  Display *display = XOpenDisplay(NULL);
  if (!display) {
    // Handle the error properly - log it and terminate
    const char *display_env = getenv("DISPLAY");
    fprintf(stderr, "ERROR: Failed to connect to X11 server. DISPLAY=%s\n", 
            display_env ? display_env : "(not set)");
    fprintf(stderr, "Make sure an X server is running and DISPLAY is set correctly.\n");
    exit(1); // Exit with error code - X11 is required
  }
  os_lnx_gfx_state->display = display;
  os_lnx_gfx_state->clipboard_arena = arena_alloc(); // Initialize clipboard arena
  
  //- dan: calculate atoms
  os_lnx_gfx_state->wm_delete_window_atom        = XInternAtom(os_lnx_gfx_state->display, "WM_DELETE_WINDOW", 0);
  os_lnx_gfx_state->wm_sync_request_atom         = XInternAtom(os_lnx_gfx_state->display, "_NET_WM_SYNC_REQUEST", 0);
  os_lnx_gfx_state->wm_sync_request_counter_atom = XInternAtom(os_lnx_gfx_state->display, "_NET_WM_SYNC_REQUEST_COUNTER", 0);
  os_lnx_gfx_state->clipboard_atom             = XInternAtom(os_lnx_gfx_state->display, "CLIPBOARD", 0);
  os_lnx_gfx_state->targets_atom               = XInternAtom(os_lnx_gfx_state->display, "TARGETS", 0);
  os_lnx_gfx_state->utf8_string_atom           = XInternAtom(os_lnx_gfx_state->display, "UTF8_STRING", 0);
  os_lnx_gfx_state->wakeup_atom                = XInternAtom(os_lnx_gfx_state->display, "_RADDBG_WAKEUP", 0);
  os_lnx_gfx_state->clipboard_target_prop_atom = XInternAtom(os_lnx_gfx_state->display, "_RADDBG_CLIPBOARD_TARGET", 0);
  
  //- dan: calculate EWMH atoms for window state
  os_lnx_gfx_state->net_wm_state_atom                = XInternAtom(os_lnx_gfx_state->display, "_NET_WM_STATE", 0);
  os_lnx_gfx_state->net_wm_state_maximized_vert_atom = XInternAtom(os_lnx_gfx_state->display, "_NET_WM_STATE_MAXIMIZED_VERT", 0);
  os_lnx_gfx_state->net_wm_state_maximized_horz_atom = XInternAtom(os_lnx_gfx_state->display, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);
  os_lnx_gfx_state->net_wm_state_hidden_atom         = XInternAtom(os_lnx_gfx_state->display, "_NET_WM_STATE_HIDDEN", 0);
  os_lnx_gfx_state->net_wm_state_fullscreen_atom     = XInternAtom(os_lnx_gfx_state->display, "_NET_WM_STATE_FULLSCREEN", 0);
  
  //- dan: create dedicated hidden window for clipboard operations
  {
    XSetWindowAttributes attributes;
    attributes.override_redirect = True; // Prevent window manager interference
    os_lnx_gfx_state->clipboard_window = XCreateWindow(
        os_lnx_gfx_state->display,
        XDefaultRootWindow(os_lnx_gfx_state->display),
        -1, -1, 1, 1, // Position off-screen, minimal size
        0, CopyFromParent, InputOnly, CopyFromParent,
        CWOverrideRedirect,
        &attributes);
    // We need to select for PropertyChangeMask to receive SelectionNotify events
    // when the property we specified in XConvertSelection is updated.
    XSelectInput(os_lnx_gfx_state->display, os_lnx_gfx_state->clipboard_window, PropertyChangeMask);
  }
  
  //- dan: initialize clipboard cache
  os_lnx_gfx_state->last_retrieved_clipboard_text = str8_lit(""); // Initial empty cache
  
  //- dan: fill out gfx info
  os_lnx_gfx_state->gfx_info.double_click_time = 0.5f;
  os_lnx_gfx_state->gfx_info.caret_blink_time = 0.5f;
  os_lnx_gfx_state->gfx_info.default_refresh_rate = 60.f;
  
  //- dan: Populate OS_Key -> KeyCode mapping
  MemoryZeroArray(os_lnx_gfx_state->keycode_from_oskey);
  Display *d = os_lnx_gfx_state->display;
  int min_keycode, max_keycode;
  XDisplayKeycodes(d, &min_keycode, &max_keycode);
  int keysyms_per_keycode = 0;
  KeySym *keysyms = XGetKeyboardMapping(d, min_keycode, (max_keycode - min_keycode + 1), &keysyms_per_keycode);
  if (keysyms != NULL)
  {
      for (int kc = min_keycode; kc <= max_keycode; ++kc)
      {
          // Check standard keysym (index 0)
          KeySym ks = XKeycodeToKeysym(d, kc, 0); 
          if (ks == XK_Shift_L)   { os_lnx_gfx_state->keycode_lshift = kc; }
          if (ks == XK_Shift_R)   { os_lnx_gfx_state->keycode_rshift = kc; }
          if (ks == XK_Control_L) { os_lnx_gfx_state->keycode_lctrl  = kc; }
          if (ks == XK_Control_R) { os_lnx_gfx_state->keycode_rctrl  = kc; }
          if (ks == XK_Alt_L)     { os_lnx_gfx_state->keycode_lalt   = kc; }
          if (ks == XK_Alt_R)     { os_lnx_gfx_state->keycode_ralt   = kc; }
          if (ks != NoSymbol)
          {
              OS_Key oskey = os_lnx_keysym_to_oskey(ks);
              if (oskey != OS_Key_Null && os_lnx_gfx_state->keycode_from_oskey[oskey] == 0)
              {
                  // Prioritize non-modifier keys or left-side modifiers for the main table
                  if (oskey != OS_Key_Shift && oskey != OS_Key_Ctrl && oskey != OS_Key_Alt) {
                      os_lnx_gfx_state->keycode_from_oskey[oskey] = kc;
                  } else if (ks == XK_Shift_L || ks == XK_Control_L || ks == XK_Alt_L) {
                      os_lnx_gfx_state->keycode_from_oskey[oskey] = kc; 
                  }
              }
          }
      }
      XFree(keysyms);
  }
  
  //- dan: load cursors
  os_lnx_gfx_state->cursors[OS_Cursor_Pointer]        = XCreateFontCursor(os_lnx_gfx_state->display, XC_left_ptr);
  os_lnx_gfx_state->cursors[OS_Cursor_IBar]           = XCreateFontCursor(os_lnx_gfx_state->display, XC_xterm);
  os_lnx_gfx_state->cursors[OS_Cursor_LeftRight]      = XCreateFontCursor(os_lnx_gfx_state->display, XC_sb_h_double_arrow);
  os_lnx_gfx_state->cursors[OS_Cursor_UpDown]         = XCreateFontCursor(os_lnx_gfx_state->display, XC_sb_v_double_arrow);
  os_lnx_gfx_state->cursors[OS_Cursor_DownRight]      = XCreateFontCursor(os_lnx_gfx_state->display, XC_bottom_right_corner);
  os_lnx_gfx_state->cursors[OS_Cursor_UpRight]        = XCreateFontCursor(os_lnx_gfx_state->display, XC_top_right_corner);
  os_lnx_gfx_state->cursors[OS_Cursor_UpDownLeftRight]= XCreateFontCursor(os_lnx_gfx_state->display, XC_fleur);
  os_lnx_gfx_state->cursors[OS_Cursor_HandPoint]      = XCreateFontCursor(os_lnx_gfx_state->display, XC_hand2);
  os_lnx_gfx_state->cursors[OS_Cursor_Disabled]       = XCreateFontCursor(os_lnx_gfx_state->display, XC_pirate);
  os_set_cursor(OS_Cursor_Pointer);
}

////////////////////////////////
//~ dan: @os_hooks Graphics System Info (Implemented Per-OS)

internal OS_GfxInfo *
os_get_gfx_info(void)
{
  return &os_lnx_gfx_state->gfx_info;
}

////////////////////////////////
//~ dan: @os_hooks Clipboards (Implemented Per-OS)

internal void
os_set_clipboard_text(String8 string)
{
  Display *d = os_lnx_gfx_state->display;
  // Arena clear/re-push might be needed if clipboard_buffer is reused often
  arena_clear(os_lnx_gfx_state->clipboard_arena); // Use clipboard_arena
  os_lnx_gfx_state->clipboard_buffer = push_str8_copy(os_lnx_gfx_state->clipboard_arena, string);
  XSetSelectionOwner(d, os_lnx_gfx_state->clipboard_atom, os_lnx_gfx_state->clipboard_window, CurrentTime);
  // Ensure the server processes the request
  XFlush(d);
}

internal String8
os_get_clipboard_text(Arena *arena)
{
  Display *d = os_lnx_gfx_state->display;
  Window owner = XGetSelectionOwner(d, os_lnx_gfx_state->clipboard_atom);
  String8 result = {0}; // Initialize result

  if (owner == None)
  {
    // No owner, clear our cache if it wasn't already empty
    if (os_lnx_gfx_state->last_retrieved_clipboard_text.size > 0) {
        // Clear or manage the arena holding the cache if necessary
        arena_clear(os_lnx_gfx_state->clipboard_arena); // Use clipboard_arena
        os_lnx_gfx_state->last_retrieved_clipboard_text = str8_lit("");
    }
    // Return empty string as there is no owner
    result = str8_lit(""); 
  }
  else if (owner == os_lnx_gfx_state->clipboard_window)
  {
    // We are the owner, return the buffer we set.
    result = push_str8_copy(arena, os_lnx_gfx_state->clipboard_buffer);
  }
  else
  {
    // Request the selection data from the current owner.
    // The result will be placed in the property 'clipboard_target_prop_atom' 
    // on our dedicated 'clipboard_window'.
    // The 'os_get_events' loop will handle the PropertyNotify event when the data arrives.
    XConvertSelection(d,
                      os_lnx_gfx_state->clipboard_atom,      // selection atom (CLIPBOARD)
                      os_lnx_gfx_state->utf8_string_atom,    // target format (UTF8_STRING)
                      os_lnx_gfx_state->clipboard_target_prop_atom, // property to store result on requestor
                      os_lnx_gfx_state->clipboard_window,    // requestor window
                      CurrentTime);
    // NOTE(perf): Cannot block here waiting for clipboard data. Must return cached value
    // and handle PropertyNotify event asynchronously.
    XFlush(d);

    // Immediately return the last successfully retrieved text from our cache.
    // The cache will be updated asynchronously later in the event loop.
    result = push_str8_copy(arena, os_lnx_gfx_state->last_retrieved_clipboard_text);
  }
  
  return result;
}

////////////////////////////////
//~ dan: @os_hooks Windows (Implemented Per-OS)

internal OS_Handle
os_window_open(Vec2F32 resolution, OS_WindowFlags flags, String8 title)
{
  //- dan: allocate window
  OS_LNX_Window *w = os_lnx_gfx_state->free_window;
  if(w)
  {
    SLLStackPop(os_lnx_gfx_state->free_window);
  }
  else
  {
    w = push_array_no_zero(os_lnx_gfx_state->arena, OS_LNX_Window, 1);
  }
  MemoryZeroStruct(w);
  DLLPushBack(os_lnx_gfx_state->first_window, os_lnx_gfx_state->last_window, w);
  
  //- dan: create window & equip with x11 info
  w->window = XCreateWindow(os_lnx_gfx_state->display,
                            XDefaultRootWindow(os_lnx_gfx_state->display),
                            0, 0, resolution.x, resolution.y,
                            0,
                            CopyFromParent,
                            InputOutput,
                            CopyFromParent,
                            0,
                            0);
  XSelectInput(os_lnx_gfx_state->display, w->window,
               ExposureMask|
               PointerMotionMask|
               ButtonPressMask|
               ButtonReleaseMask|
               KeyPressMask|
               KeyReleaseMask|
               FocusChangeMask);
  
  //- dan: attach name
  Temp scratch = scratch_begin(0, 0);
  String8 title_copy = push_str8_copy(scratch.arena, title);
  // XStoreName(os_lnx_gfx_state->display, w->window, (char *)title_copy.str); // MOVED
  // scratch_end(scratch); // MOVED
  
  //--- START NEW GLX/GLEW INIT CODE ---
  Display *d = os_lnx_gfx_state->display;
  int default_screen = XDefaultScreen(d);

  // Define desired framebuffer attributes
  static int visual_attribs[] = {
    GLX_X_RENDERABLE, True,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_ALPHA_SIZE, 8,
    GLX_DEPTH_SIZE, 24,
    GLX_STENCIL_SIZE, 8,
    GLX_DOUBLEBUFFER, True,
    //GLX_SAMPLE_BUFFERS  , 1, // For MSAA
    //GLX_SAMPLES         , 4, // MSAA samples
    None // Null terminate the list
  };

  // Choose FBConfig
  int fbcount;
  GLXFBConfig *fbc = glXChooseFBConfig(d, default_screen, visual_attribs, &fbcount);
  if (!fbc || fbcount <= 0) {
    fprintf(stderr, "Failed to retrieve a framebuffer config\n");
    XDestroyWindow(d, w->window); // Destroy the initial dummy window
    // Handle error: maybe release 'w' back to free list?
    DLLRemove(os_lnx_gfx_state->first_window, os_lnx_gfx_state->last_window, w);
    SLLStackPush(os_lnx_gfx_state->free_window, w);
    return os_handle_zero();
  }
  // Select the best FBConfig (take the first one for simplicity)
  GLXFBConfig best_fbc = fbc[0];
  XFree(fbc); // Free the list returned by glXChooseFBConfig
  os_lnx_gfx_state->fb_config = best_fbc; // Store if needed globally

  // Get visual info from FBConfig
  XVisualInfo *vi = glXGetVisualFromFBConfig(d, best_fbc);
  if (!vi) {
    fprintf(stderr, "Could not create visual window info\n");
    XDestroyWindow(d, w->window); // Destroy the initial dummy window
    DLLRemove(os_lnx_gfx_state->first_window, os_lnx_gfx_state->last_window, w);
    SLLStackPush(os_lnx_gfx_state->free_window, w);
    return os_handle_zero();
  }

  // Destroy the initially created window (it had the wrong visual)
  XDestroyWindow(d, w->window);

  // Create a colormap and window attributes
  Colormap cmap = XCreateColormap(d, XRootWindow(d, vi->screen), vi->visual, AllocNone);
  XSetWindowAttributes swa;
  swa.colormap = cmap;
  swa.background_pixmap = None;
  swa.border_pixel = 0;
  swa.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask |
                   ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask;

  // Re-create the window with the correct visual and attributes
  w->window = XCreateWindow(d,
                          XRootWindow(d, vi->screen),
                          0, 0, resolution.x, resolution.y,
                          0, // border_width
                          vi->depth,
                          InputOutput,
                          vi->visual,
                          CWBorderPixel | CWColormap | CWEventMask, // Value mask
                          &swa);
  if (!w->window) {
      fprintf(stderr, "Failed to create window with chosen visual\n");
      XFreeColormap(d, cmap);
      XFree(vi);
      DLLRemove(os_lnx_gfx_state->first_window, os_lnx_gfx_state->last_window, w);
      SLLStackPush(os_lnx_gfx_state->free_window, w);
      return os_handle_zero();
  }
  // Free the visual info structure
  XFree(vi);
  XFreeColormap(d, cmap); // Colormap is associated with window now

  // Set window title AFTER final window creation
  XStoreName(os_lnx_gfx_state->display, w->window, (char *)title_copy.str);
  scratch_end(scratch);

  // Create temporary legacy OpenGL context first for GLEW initialization
  // We still need a temporary context to initialize GLEW and get the glXGetProcAddressARB function
  GLXContext temp_ctx = glXCreateNewContext(d, best_fbc, GLX_RGBA_TYPE, 0, True);
  if (!temp_ctx) {
      fprintf(stderr, "Failed to create temporary GL context\\n");
      XDestroyWindow(d, w->window);
      DLLRemove(os_lnx_gfx_state->first_window, os_lnx_gfx_state->last_window, w);
      SLLStackPush(os_lnx_gfx_state->free_window, w);
      return os_handle_zero();
  }

  // Make temporary context current
  if (!glXMakeCurrent(d, w->window, temp_ctx)) {
      fprintf(stderr, "Failed to make temporary context current\\n");
      glXDestroyContext(d, temp_ctx);
      XDestroyWindow(d, w->window);
      DLLRemove(os_lnx_gfx_state->first_window, os_lnx_gfx_state->last_window, w);
      SLLStackPush(os_lnx_gfx_state->free_window, w);
      return os_handle_zero();
  }

  fprintf(stderr, "Temporary OpenGL context created and made current\n");
  
  // We now have a valid temporary context, but don't set has_valid_context yet
  // as we want to initialize GLEW first
  
  // Initialize GLEW AFTER making the temporary context current
  r_gl_init_glew_if_needed(); // Only initialize GLEW here, not resources!
  
  // Attempt to get the modern context creation function pointer
  PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = NULL;
  glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)
      glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");

  GLXContext final_ctx = NULL;
  // Use the flag set by r_gl_init_extensions_if_needed
  if(glXCreateContextAttribsARB)
  {
    int context_attribs[] = {
      GLX_CONTEXT_MAJOR_VERSION_ARB, 4, // Request OpenGL 4.6
      GLX_CONTEXT_MINOR_VERSION_ARB, 6,
      GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
      // GLX_CONTEXT_FLAGS_ARB , GLX_CONTEXT_DEBUG_BIT_ARB, // Optional debug context
      GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
      None // Null terminate
    };

    // glXCreateContextAttribsARB is defined by glew.h if GLEW_ARB_create_context is defined
    final_ctx = glXCreateContextAttribsARB(d, best_fbc, temp_ctx, True, context_attribs);

    if (!final_ctx) {
      fprintf(stderr, "Failed to create GL 3.3 context using ARB (glXGetProcAddressARB succeeded), falling back to legacy context.\\n");
      final_ctx = temp_ctx; // Keep the temporary context
      temp_ctx = NULL;      // Don't destroy it later
    } else {
      // Successfully created 3.3 context, destroy the temporary one
      glXMakeCurrent(d, None, NULL); // Make no context current before destroying temp
      // glXDestroyContext(d, temp_ctx); // TEMP: Comment out to test resource sharing
      glXDestroyContext(d, temp_ctx); // Destroy temporary context now that we have a working final context
      temp_ctx = NULL; // Mark temp as destroyed

      // Make the new 3.3 context current
      if (!glXMakeCurrent(d, w->window, final_ctx)) {
         fprintf(stderr, "Failed to make final GL 3.3 context current\\n");
         glXDestroyContext(d, final_ctx);
         XDestroyWindow(d, w->window);
         DLLRemove(os_lnx_gfx_state->first_window, os_lnx_gfx_state->last_window, w);
         SLLStackPush(os_lnx_gfx_state->free_window, w);
         return os_handle_zero();
      }
      
      // Set the context as valid - THIS IS NEW
      r_gl_set_has_valid_context(1);
      
      // Now that the final context is current, create all OpenGL resources
      r_gl_create_global_resources();
    }
  }
  else
  {
    fprintf(stderr, "glXCreateContextAttribsARB not available. Using legacy context.\n");
    final_ctx = temp_ctx; // Keep the temporary context
    temp_ctx = NULL;      // Don't destroy it later
    // The temporary context is already current
    
    // Set the context as valid - THIS IS NEW for legacy path
    r_gl_set_has_valid_context(1); 

    // REMOVED: Create OpenGL resources in the legacy context
    // r_gl_create_global_resources();
  }

  // Store the final context (either the new 3.3 or the fallback legacy one)
  w->gl_context = final_ctx;
 
  // ADDED: Log the final OpenGL version *after* the context is made current
  if (glXGetCurrentContext() == final_ctx) {
      const GLubyte* version = glGetString(GL_VERSION);
      fprintf(stderr, ">>> Final OpenGL Context Version: %s\n", version ? (const char*)version : "NULL (Error getting version?)");
  } else {
      fprintf(stderr, ">>> ERROR: Final context not current after creation!\n");
  }

  // Resume existing window setup logic
  XSelectInput(os_lnx_gfx_state->display, w->window,
               ExposureMask|
               PointerMotionMask|
               ButtonPressMask|
               ButtonReleaseMask|
               KeyPressMask|
               KeyReleaseMask|
               FocusChangeMask);
  Atom protocols[] =
  {
    os_lnx_gfx_state->wm_delete_window_atom,
    os_lnx_gfx_state->wm_sync_request_atom,
  };
  XSetWMProtocols(os_lnx_gfx_state->display, w->window, protocols, ArrayCount(protocols));
  {
    XSyncValue initial_value;
    XSyncIntToValue(&initial_value, 0);
    w->counter_xid = XSyncCreateCounter(os_lnx_gfx_state->display, initial_value);
  }
  XChangeProperty(os_lnx_gfx_state->display, w->window, os_lnx_gfx_state->wm_sync_request_counter_atom, XA_CARDINAL, 32, PropModeReplace, (U8 *)&w->counter_xid, 1);
  
  //- dan: convert to handle & return
  OS_Handle handle = {(U64)w};
  return handle;
}

internal void
os_window_close(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  Display *d = os_lnx_gfx_state->display;

  if (w != 0 && d != 0)
  {
    // Destroy the XSync counter associated with the window
    if (w->counter_xid != None) // Check if counter was created
    {
      XSyncDestroyCounter(d, w->counter_xid);
      if (XSyncDestroyCounter(d, w->counter_xid) == BadValue) { // Check return value
          fprintf(stderr, "Warning: Failed to destroy XSync counter %lu.\n", (unsigned long)w->counter_xid);
      }
      w->counter_xid = None; // Mark as destroyed
    }

    // Make context not current and destroy it
    if (w->gl_context) {
        // Check if this context is current before making null current
        if (glXGetCurrentContext() == w->gl_context) {
            glXMakeCurrent(d, None, NULL);
        }
        glXDestroyContext(d, w->gl_context);
        w->gl_context = NULL; // Mark as destroyed
    }

    // Destroy the X11 window
    XDestroyWindow(d, w->window);

    // Remove from the active window list
    DLLRemove(os_lnx_gfx_state->first_window, os_lnx_gfx_state->last_window, w);

    // Add to the free list
    SLLStackPush(os_lnx_gfx_state->free_window, w);

    // Maybe flush needed? Depends if caller expects immediate effect.
    // XFlush(d);
  }
}

internal void
os_window_first_paint(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  XMapWindow(os_lnx_gfx_state->display, w->window);
}

internal void
os_window_focus(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window*)handle.u64[0];
  Display *d = os_lnx_gfx_state->display;
  if (w != 0 && d != 0)
  {
    // Raise the window to the top
    XRaiseWindow(d, w->window);
    
    // Set the input focus
    // RevertToParent means focus goes to parent if window becomes unviewable.
    XSetInputFocus(d, w->window, RevertToParent, CurrentTime);
    
    // Ensure the server processes the requests
    XFlush(d);
  }
}

internal B32
os_window_is_focused(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
  OS_LNX_Window *w = (OS_LNX_Window*)handle.u64[0];
  return w->has_focus;
}

internal B32
os_window_is_fullscreen(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
  OS_LNX_Window *w = (OS_LNX_Window*)handle.u64[0];
  Display *d = os_lnx_gfx_state->display;
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  Atom *props = 0;
  B32 is_fullscreen = 0;

  // Check _NET_WM_STATE property for the _NET_WM_STATE_FULLSCREEN atom
  int result = XGetWindowProperty(d,
                                w->window,
                                os_lnx_gfx_state->net_wm_state_atom,
                                0, (~0L), // Read the whole property
                                False,    // Don't delete
                                XA_ATOM,  // Expected type is Atom
                                &actual_type, &actual_format,
                                &nitems, &bytes_after, (unsigned char **)&props);

  if (result == Success && props != 0)
  {
    for (unsigned long i = 0; i < nitems; i++)
    {
      if (props[i] == os_lnx_gfx_state->net_wm_state_fullscreen_atom)
      {
        is_fullscreen = 1;
        break;
      }
    }
    XFree(props);
  }
  return is_fullscreen;
}

internal void
os_window_set_fullscreen(OS_Handle handle, B32 fullscreen)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window*)handle.u64[0];
  Display *d = os_lnx_gfx_state->display;
  Window root = XDefaultRootWindow(d);

  // EWMH action: _NET_WM_STATE_REMOVE=0, _NET_WM_STATE_ADD=1
  long action = fullscreen ? 1 : 0;

  XEvent e = {0};
  e.type = ClientMessage;
  e.xclient.window = w->window;
  e.xclient.message_type = os_lnx_gfx_state->net_wm_state_atom;
  e.xclient.format = 32;
  e.xclient.data.l[0] = action; // Add or Remove state
  e.xclient.data.l[1] = os_lnx_gfx_state->net_wm_state_fullscreen_atom;
  e.xclient.data.l[2] = 0; // No second property
  e.xclient.data.l[3] = 1; // Source indication: Normal Application
  e.xclient.data.l[4] = 0;

  // Send the event to the root window, requesting the WM change the state
  XSendEvent(d, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &e);
  XFlush(d); // Ensure the request is sent immediately
}

internal B32
os_window_is_maximized(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
  OS_LNX_Window *w = (OS_LNX_Window*)handle.u64[0];
  Display *d = os_lnx_gfx_state->display;
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  Atom *props = 0;
  B32 is_maximized = 0;

  int result = XGetWindowProperty(d,
                                w->window,
                                os_lnx_gfx_state->net_wm_state_atom,
                                0, (~0L), // Read the whole property
                                False,    // Don't delete
                                XA_ATOM,  // Expected type is Atom
                                &actual_type, &actual_format,
                                &nitems, &bytes_after, (unsigned char **)&props);

  if (result == Success && props != 0)
  {
    B32 found_vert = 0;
    B32 found_horz = 0;
    for (unsigned long i = 0; i < nitems; i++)
    {
      if (props[i] == os_lnx_gfx_state->net_wm_state_maximized_vert_atom)
      {
        found_vert = 1;
      }
      else if (props[i] == os_lnx_gfx_state->net_wm_state_maximized_horz_atom)
      {
        found_horz = 1;
      }
    }
    is_maximized = found_vert && found_horz;
    XFree(props);
  }

  return is_maximized;
}

internal void
os_window_set_maximized(OS_Handle handle, B32 maximized)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window*)handle.u64[0];
  Display *d = os_lnx_gfx_state->display;
  Window root = XDefaultRootWindow(d);

  // Action: _NET_WM_STATE_REMOVE=0, _NET_WM_STATE_ADD=1, _NET_WM_STATE_TOGGLE=2
  long action = maximized ? 1 : 0;

  XEvent e = {0};
  e.type = ClientMessage;
  e.xclient.window = w->window;
  e.xclient.message_type = os_lnx_gfx_state->net_wm_state_atom;
  e.xclient.format = 32;
  e.xclient.data.l[0] = action; // Add or Remove state
  e.xclient.data.l[1] = os_lnx_gfx_state->net_wm_state_maximized_horz_atom;
  e.xclient.data.l[2] = os_lnx_gfx_state->net_wm_state_maximized_vert_atom;
  e.xclient.data.l[3] = 0; // Source indication (0 for normal apps)
  e.xclient.data.l[4] = 0;

  XSendEvent(d, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &e);
  XFlush(d);
}

internal B32
os_window_is_minimized(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
  OS_LNX_Window *w = (OS_LNX_Window*)handle.u64[0];
  Display *d = os_lnx_gfx_state->display;
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  Atom *props = 0;
  B32 is_hidden = 0;

  int result = XGetWindowProperty(d,
                                w->window,
                                os_lnx_gfx_state->net_wm_state_atom,
                                0, (~0L),
                                False,
                                XA_ATOM,
                                &actual_type, &actual_format,
                                &nitems, &bytes_after, (unsigned char **)&props);

  if (result == Success && props != 0)
  {
    for (unsigned long i = 0; i < nitems; i++)
    {
      if (props[i] == os_lnx_gfx_state->net_wm_state_hidden_atom)
      {
        is_hidden = 1;
        break;
      }
    }
    XFree(props);
  }
  // Additionally, check the classic WM_STATE IconicState for compatibility?
  // Some older WMs might not fully support EWMH _NET_WM_STATE_HIDDEN.
  // Atom wm_state = XInternAtom(d, "WM_STATE", False);
  // ... query WM_STATE property ... check for IconicState ...
  // For now, just rely on EWMH.

  return is_hidden;
}

internal void
os_window_set_minimized(OS_Handle handle, B32 minimized)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window*)handle.u64[0];
  Display *d = os_lnx_gfx_state->display;

  if (minimized)
  {
    // Request the window manager to iconify (minimize) the window
    XIconifyWindow(d, w->window, DefaultScreen(d));
  }
  else
  {
    // Request the window manager to remove the hidden state (un-minimize)
    // using EWMH _NET_WM_STATE message.
    Window root = XDefaultRootWindow(d);
    XEvent e = {0};
    e.type = ClientMessage;
    e.xclient.window = w->window;
    e.xclient.message_type = os_lnx_gfx_state->net_wm_state_atom;
    e.xclient.format = 32;
    e.xclient.data.l[0] = 0; // _NET_WM_STATE_REMOVE
    e.xclient.data.l[1] = os_lnx_gfx_state->net_wm_state_hidden_atom;
    e.xclient.data.l[2] = 0; // No second property
    e.xclient.data.l[3] = 1; // Source indication: Normal Application
    e.xclient.data.l[4] = 0;

    XSendEvent(d, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &e);

    // Ensure the window is mapped and raised
    XMapRaised(d, w->window);
  }
  XFlush(d);
}

internal void
os_window_bring_to_front(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window*)handle.u64[0];
  Display *d = os_lnx_gfx_state->display;
  if (w != 0 && d != 0)
  {
    XRaiseWindow(d, w->window);
    XFlush(d);
  }
}

internal void
os_window_set_monitor(OS_Handle handle, OS_Handle monitor)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window*)handle.u64[0];
  Display *d = os_lnx_gfx_state->display;
  Window root = XDefaultRootWindow(d);
  RROutput output_id = (RROutput)monitor.u64[0];

  if (w == 0 || d == 0 || output_id == None) {
    return;
  }

  // Get target monitor geometry
  Rng2S32 monitor_rect = {0};
  int monitor_width = 0;
  int monitor_height = 0;
  XRRScreenResources *res = XRRGetScreenResourcesCurrent(d, root);
  if (res) {
      XRROutputInfo *output_info = XRRGetOutputInfo(d, res, output_id);
      if (output_info && output_info->crtc != None) {
          XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(d, res, output_info->crtc);
          if (crtc_info) {
              monitor_rect = r2s32p(crtc_info->x, crtc_info->y,
                                   crtc_info->x + crtc_info->width,
                                   crtc_info->y + crtc_info->height);
              monitor_width = crtc_info->width;
              monitor_height = crtc_info->height;
              XRRFreeCrtcInfo(crtc_info);
          }
      }
      if (output_info) XRRFreeOutputInfo(output_info);
      XRRFreeScreenResources(res);
  }

  // Check if monitor info was successfully obtained
  if (monitor_width <= 0 || monitor_height <= 0) {
      return; // Failed to get valid monitor geometry
  }

  // Get current window geometry (size only needed)
  Window root_return;
  int win_x_ignored, win_y_ignored;
  unsigned int win_width, win_height, win_border_ignored, win_depth_ignored;
  if (!XGetGeometry(d, w->window, &root_return, &win_x_ignored, &win_y_ignored, &win_width, &win_height, &win_border_ignored, &win_depth_ignored)) {
      return; // Failed to get window geometry
  }

  // Calculate centered position on the target monitor
  S32 target_x = monitor_rect.x0 + (monitor_width / 2) - ((S32)win_width / 2);
  S32 target_y = monitor_rect.y0 + (monitor_height / 2) - ((S32)win_height / 2);

  // Move the window
  XMoveWindow(d, w->window, target_x, target_y);
  XFlush(d); // Ensure the request is sent
}

internal void
os_window_clear_custom_border_data(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal void
os_window_push_custom_title_bar(OS_Handle handle, F32 thickness)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal void
os_window_push_custom_edges(OS_Handle handle, F32 thickness)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal void
os_window_push_custom_title_bar_client_area(OS_Handle handle, Rng2F32 rect)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal Rng2F32
os_rect_from_window(OS_Handle handle)
{
  Rng2F32 result = r2f32p(0, 0, 0, 0);
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  if (w != 0)
  {
    Display *d = os_lnx_gfx_state->display;
    Window root = XDefaultRootWindow(d);
    int x, y;
    unsigned int width, height, border_width, depth;
    Window child_return;

    // Get window geometry (width, height)
    if (XGetGeometry(d, w->window, &root, &x, &y, &width, &height, &border_width, &depth))
    {
       // Get window position relative to root
       if (XTranslateCoordinates(d, w->window, root, 0, 0, &x, &y, &child_return))
       {
           result.x0 = (F32)x;
           result.y0 = (F32)y;
           result.x1 = (F32)(x + width);
           result.y1 = (F32)(y + height);
       }
    }
  }
  return result;
}

internal Rng2F32
os_client_rect_from_window(OS_Handle handle)
{
  Rng2F32 result = r2f32p(0, 0, 0, 0);
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  if (w != 0)
  {
    Display *d = os_lnx_gfx_state->display;
    Window root_return;
    int x_return, y_return;
    unsigned int width, height, border_width_return, depth_return;

    if (XGetGeometry(d, w->window, &root_return, &x_return, &y_return, &width, &height, &border_width_return, &depth_return))
    {
       // Client rect is always 0,0 to width, height relative to the window itself
       result.x0 = 0;
       result.y0 = 0;
       result.x1 = (F32)width;
       result.y1 = (F32)height;
    }
  }
  return result;
}

internal F32
os_dpi_from_window(OS_Handle handle)
{
    OS_LNX_Window *w = (OS_LNX_Window*)handle.u64[0];
    if (w != 0 && w->dpi > 0.0f) // Check if DPI is already cached
    {
        return w->dpi;
    }

    // Default DPI
    F32 dpi = 96.0f;
    Display *d = os_lnx_gfx_state->display;
    char *resource_manager = XResourceManagerString(d);
    Temp scratch = scratch_begin(0, 0); // Begin scratch arena

    if (resource_manager)
    {
        XrmDatabase db = XrmGetStringDatabase(resource_manager);
        if (db)
        {
            char *type = NULL;
            XrmValue value;
            // Try to get Xft.dpi
            if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value) == True)
            {
                if (value.addr && value.size > 0)
                {
                    // Convert string value to float
                    // Use scratch arena for temporary allocation
                    char *str_dpi = push_array(scratch.arena, char, value.size + 1);
                    if(str_dpi)
                    {
                        MemoryCopy(str_dpi, value.addr, value.size); // Use MemoryCopy for safety
                        str_dpi[value.size] = '\0';
                        dpi = atof(str_dpi);
                        // No MemoryFree needed, scratch arena handles cleanup
                        // Basic validation: DPI shouldn't be zero or negative
                        if (dpi <= 0) { dpi = 96.0f; }
                    }
                }
            }
            else
            {
                // Fallback: Calculate DPI from screen dimensions if Xft.dpi is not found
                int screen_num = DefaultScreen(d);
                int width_px = DisplayWidth(d, screen_num);
                int width_mm = DisplayWidthMM(d, screen_num);
                if (width_mm > 0)
                {
                    dpi = (F32)(width_px * 25.4 / width_mm);
                    if (dpi <= 0) { dpi = 96.0f; } // Sanity check
                }
            }
            XrmDestroyDatabase(db);
        }
    }
    else
    {
        // Fallback if no resource manager string: Calculate DPI from screen dimensions
        int screen_num = DefaultScreen(d);
        int width_px = DisplayWidth(d, screen_num);
        int width_mm = DisplayWidthMM(d, screen_num);
        if (width_mm > 0)
        {
            dpi = (F32)(width_px * 25.4 / width_mm);
            if (dpi <= 0) { dpi = 96.0f; } // Sanity check
        }
    }
    
    scratch_end(scratch); // End scratch arena scope

    if (w != 0) // Cache the calculated DPI if window is valid
    {
        w->dpi = dpi;
    }

    return dpi;
}

////////////////////////////////
//~ dan: @os_hooks Monitors (Implemented Per-OS)

internal OS_HandleArray
os_push_monitors_array(Arena *arena)
{
  OS_HandleArray array = {0};
  OS_HandleList list = {0};
  Display *d = os_lnx_gfx_state->display;
  Window root = XDefaultRootWindow(d);
  
  XRRScreenResources *res = XRRGetScreenResourcesCurrent(d, root);
  if (res == NULL)
  {
    return array; // Failed to get resources
  }
  
  for (int i = 0; i < res->noutput; i++)
  {
    XRROutputInfo *output_info = XRRGetOutputInfo(d, res, res->outputs[i]);
    if (output_info == NULL)
    {
      continue;
    }
    
    // Check if the output is connected and has an active CRTC (is an active monitor)
    if (output_info->connection == RR_Connected && output_info->crtc != None)
    {
      // Consider this an active monitor
      // Use the RROutput ID as the handle value
      OS_Handle monitor_handle = {(U64)res->outputs[i]};
      os_handle_list_push(arena, &list, monitor_handle);
    }
    
    XRRFreeOutputInfo(output_info);
  }
  
  XRRFreeScreenResources(res);
  
  array = os_handle_array_from_list(arena, &list);
  return array;
}

internal OS_Handle
os_primary_monitor(void)
{
  OS_Handle result = {0};
  Display *d = os_lnx_gfx_state->display;
  Window root = XDefaultRootWindow(d);
  
  // Use Xrandr to get the primary output
  RROutput primary_output = XRRGetOutputPrimary(d, root);
  
  if (primary_output != None)
  {
    // Assign the RROutput ID to the handle
    result.u64[0] = (U64)primary_output;
  }
  // else: primary_output is None, meaning no primary output is set or Xrandr failed.
  // Return the zero handle in this case.
  
  return result;
}

internal OS_Handle
os_monitor_from_window(OS_Handle window)
{
  OS_Handle result = {0};
  OS_LNX_Window *w = (OS_LNX_Window*)window.u64[0];
  Display *d = os_lnx_gfx_state->display;

  if (w == 0 || d == 0) {
    return result; // Return zero handle if window or display is invalid
  }

  Window root = XDefaultRootWindow(d);
  int win_x, win_y;
  unsigned int win_width, win_height, win_border, win_depth;
  Window child_return;

  // 1. Get window geometry
  if (!XGetGeometry(d, w->window, &root, &win_x, &win_y, &win_width, &win_height, &win_border, &win_depth)) {
    return result; // Failed to get window geometry
  }
  // Translate coordinates relative to root
  if (!XTranslateCoordinates(d, w->window, root, 0, 0, &win_x, &win_y, &child_return)) {
    return result; // Failed to translate coordinates
  }
  Rng2S32 window_rect = r2s32p(win_x, win_y, win_x + win_width, win_y + win_height);

  // 2. Get monitor resources
  XRRScreenResources *res = XRRGetScreenResourcesCurrent(d, root);
  if (res == NULL) {
    return result; // Failed to get screen resources
  }

  RROutput best_output = None;
  S64 max_intersection_area = -1;

  // 3. Iterate through monitors
  for (int i = 0; i < res->noutput; i++) {
    XRROutputInfo *output_info = XRRGetOutputInfo(d, res, res->outputs[i]);
    if (output_info == NULL) {
      continue;
    }

    // Consider only connected outputs with an active CRTC
    if (output_info->connection == RR_Connected && output_info->crtc != None) {
      XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(d, res, output_info->crtc);
      if (crtc_info != NULL) {
        // 4. Get monitor geometry
        Rng2S32 monitor_rect = r2s32p(crtc_info->x, crtc_info->y,
                                     crtc_info->x + crtc_info->width,
                                     crtc_info->y + crtc_info->height);

        // 5. Calculate intersection area
        Rng2S32 intersection = intersect_2s32(window_rect, monitor_rect);
        S64 area = (S64)dim_2s32(intersection).x * (S64)dim_2s32(intersection).y;

        if (area > max_intersection_area) {
          max_intersection_area = area;
          best_output = res->outputs[i];
        }
        XRRFreeCrtcInfo(crtc_info);
      }
    }
    XRRFreeOutputInfo(output_info);
  }

  XRRFreeScreenResources(res);

  // 6. Return handle of the best monitor
  if (best_output != None) {
    result.u64[0] = (U64)best_output;
  }
  // If no intersection found, or only disconnected monitors, result remains zero handle.

  return result;
}

internal String8
os_name_from_monitor(Arena *arena, OS_Handle monitor)
{
  String8 result = {0};
  Display *d = os_lnx_gfx_state->display;
  Window root = XDefaultRootWindow(d);
  RROutput output_id = (RROutput)monitor.u64[0];

  if (output_id == None || d == NULL) {
    return result; // Return zero string if handle or display is invalid
  }

  XRRScreenResources *res = XRRGetScreenResourcesCurrent(d, root);
  if (res == NULL) {
    return result; // Failed to get screen resources
  }

  XRROutputInfo *output_info = XRRGetOutputInfo(d, res, output_id);
  if (output_info != NULL)
  {
    // Copy the name string into the arena
    if (output_info->name != NULL && output_info->nameLen > 0)
    {
        result = push_str8_copy(arena, str8((U8*)output_info->name, output_info->nameLen));
    }
    XRRFreeOutputInfo(output_info);
  }

  XRRFreeScreenResources(res);

  return result;
}

internal Vec2F32
os_dim_from_monitor(OS_Handle monitor)
{
  Vec2F32 result = v2f32(0, 0);
  Display *d = os_lnx_gfx_state->display;
  Window root = XDefaultRootWindow(d);
  RROutput output_id = (RROutput)monitor.u64[0];

  if (output_id == None || d == NULL) {
    return result;
  }

  XRRScreenResources *res = XRRGetScreenResourcesCurrent(d, root);
  if (res == NULL) {
    return result;
  }

  XRROutputInfo *output_info = XRRGetOutputInfo(d, res, output_id);
  if (output_info == NULL || output_info->crtc == None) {
    if (output_info) XRRFreeOutputInfo(output_info);
    XRRFreeScreenResources(res);
    return result;
  }

  XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(d, res, output_info->crtc);
  if (crtc_info != NULL)
  {
    result.x = (F32)crtc_info->width;
    result.y = (F32)crtc_info->height;
    XRRFreeCrtcInfo(crtc_info);
  }

  XRRFreeOutputInfo(output_info);
  XRRFreeScreenResources(res);

  return result;
}

////////////////////////////////
//~ dan: @os_hooks Events (Implemented Per-OS)

internal void
os_send_wakeup_event(void)
{
  Display *d = os_lnx_gfx_state->display;
  Window root = XDefaultRootWindow(d);
  XEvent event = {0};
  event.type = ClientMessage;
  event.xclient.display = d;
  event.xclient.window = root; // Sending to root, could send to specific window
  event.xclient.message_type = os_lnx_gfx_state->wakeup_atom;
  event.xclient.format = 32; // size of data items (l[0]...l[4]) in bits
  // event.xclient.data.l[0..4] can contain custom data if needed
  XSendEvent(d, root, False, NoEventMask, &event);
  XFlush(d);
}

internal OS_EventList
os_get_events(Arena *arena, B32 wait)
{
  OS_EventList evts = {0};
  Display *d = os_lnx_gfx_state->display; // Cache display pointer
  
  // Check for pending events or wait if requested
  if(XPending(d) == 0 && wait)
  {
    // No events pending, and we need to wait. Use select() on the X connection FD.
    int x11_fd = ConnectionNumber(d);
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(x11_fd, &fds);
    
    // Select blocks until the FD is readable (meaning X event is available)
    // No timeout specified, so it waits indefinitely like GetMessage.
    int select_result = select(x11_fd + 1, &fds, NULL, NULL, NULL);
    
    if (select_result < 0 && errno != EINTR) {
        // Handle select error (e.g., log it), but avoid busy-looping.
        // For now, just return empty list on error.
        perror("select");
        return evts;
    }
    // If select_result > 0 or == 0 (EINTR), we proceed to check XPending again below.
  }

  // Process all currently pending events
  while(XPending(d) > 0)
  {
    XEvent evt = {0};
    XNextEvent(d, &evt);

    // Add global modifier update here if needed, based on event state
    // Example: os_lnx_gfx_state->modifiers = os_lnx_get_modifiers_from_state(evt.xkey.state or evt.xbutton.state);

    switch(evt.type)
    {
      default:{}break;
      
      //- dan: key presses/releases
      case KeyPress:
      case KeyRelease:
      {
        // dan: determine flags
        OS_Modifiers flags = 0;
        if(evt.xkey.state & ShiftMask)   { flags |= OS_Modifier_Shift; }
        if(evt.xkey.state & ControlMask) { flags |= OS_Modifier_Ctrl; }
        if(evt.xkey.state & Mod1Mask)    { flags |= OS_Modifier_Alt; }
        
        // dan: map keycode -> keysym & possibly text
        KeySym keysym = NoSymbol;
        char text_buffer[16]; // Buffer for translated characters
        int nbytes = XLookupString(&evt.xkey, text_buffer, sizeof(text_buffer), &keysym, NULL);

        // dan: map keysym -> OS_Key
        OS_Key key = os_lnx_keysym_to_oskey(keysym);
        
        // dan: push Press/Release event
        OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xkey.window);
        if (key != OS_Key_Null) // Only push press/release for keys we map
        {
            OS_Event *press_release_evt = os_event_list_push_new(arena, &evts, evt.type == KeyPress ? OS_EventKind_Press : OS_EventKind_Release);
            press_release_evt->window.u64[0] = (U64)window;
            press_release_evt->flags = flags;
            press_release_evt->key = key;
            os_lnx_gfx_state->key_is_down[key] = (evt.type == KeyPress);
        }

        // dan: push Text event if applicable (only on KeyPress)
        if (evt.type == KeyPress && nbytes > 0)
        {
          // Filter out control characters except for common ones like tab, enter, backspace if desired
          // For simplicity, let's push most things XLookupString gives us.
          // A more robust implementation might check iscntrl() but allow specific control codes.
          if (!(flags & OS_Modifier_Ctrl) && !(flags & OS_Modifier_Alt)) // Don't push text for Ctrl/Alt combinations usually
          {
            String8 text_str = str8((U8*)text_buffer, nbytes);
            // Decode UTF-8 potentially received from XLookupString
            UnicodeDecode decoded = utf8_decode(text_str.str, text_str.size);
            U32 codepoint = decoded.codepoint; // Get codepoint directly
            if (codepoint != 0 && codepoint != 127) // Skip null and delete
            {
                OS_Event *text_evt = os_event_list_push_new(arena, &evts, OS_EventKind_Text);
                text_evt->window.u64[0] = (U64)window;
                text_evt->flags = flags; // Modifiers might be relevant for text sometimes?
                text_evt->character = codepoint;
            }
          }
        }
      }break;
      
      //- dan: mouse button presses/releases
      case ButtonPress:
      case ButtonRelease:
      {
        // dan: determine flags
        OS_Modifiers flags = 0;
        if(evt.xbutton.state & ShiftMask)   { flags |= OS_Modifier_Shift; }
        if(evt.xbutton.state & ControlMask) { flags |= OS_Modifier_Ctrl; }
        if(evt.xbutton.state & Mod1Mask)    { flags |= OS_Modifier_Alt; }
        
        // dan: map button -> OS_Key
        OS_Key key = OS_Key_Null;
        switch(evt.xbutton.button)
        {
          default:{}break;
          case Button1:{key = OS_Key_LeftMouseButton;}break;
          case Button2:{key = OS_Key_MiddleMouseButton;}break;
          case Button3:{key = OS_Key_RightMouseButton;}break;
          case Button4: // Scroll up
          case Button5: // Scroll down
          {
             OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xbutton.window);
             OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_Scroll);
             e->window.u64[0] = (U64)window;
             e->flags = flags;
             e->pos.x = (F32)evt.xbutton.x;
             e->pos.y = (F32)evt.xbutton.y;
             e->delta.y = (evt.xbutton.button == Button4) ? 120.f : -120.f; // Simulate standard wheel delta
          } break;
        }
        
        if (key != OS_Key_Null)
        {
            // dan: push event
            OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
            OS_Event *e = os_event_list_push_new(arena, &evts, evt.type == ButtonPress ? OS_EventKind_Press : OS_EventKind_Release);
            e->window.u64[0] = (U64)window;
            e->flags = flags;
            e->key = key;
            os_lnx_gfx_state->key_is_down[key] = (evt.type == ButtonPress);
        }
      }break;
      
      //- dan: mouse motion
      case MotionNotify:
      {
        OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
        OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_MouseMove);
        e->window.u64[0] = (U64)window;
        e->pos.x = (F32)evt.xmotion.x;
        e->pos.y = (F32)evt.xmotion.y;
      }break;
      
      //- dan: window focus/unfocus
      case FocusIn:
      case FocusOut:
      {
         OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xfocus.window);
         if (window) {
             window->has_focus = (evt.type == FocusIn);
             if (evt.type == FocusOut) {
                 OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_WindowLoseFocus);
                 e->window.u64[0] = (U64)window;
                 // Clear key state on focus lost? Maybe not necessary if we query directly.
             }
         }
      }break;
      
      //- dan: client messages
      case ClientMessage:
      {
        if((Atom)evt.xclient.data.l[0] == os_lnx_gfx_state->wm_delete_window_atom)
        {
          OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
          OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_WindowClose);
          e->window.u64[0] = (U64)window;
        }
        else if((Atom)evt.xclient.data.l[0] == os_lnx_gfx_state->wm_sync_request_atom)
        {
          OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
          if(window != 0)
          {
            window->counter_value = 0;
            window->counter_value |= evt.xclient.data.l[2];
            window->counter_value |= (evt.xclient.data.l[3] << 32);
            XSyncValue value;
            XSyncIntToValue(&value, window->counter_value);
            XSyncSetCounter(os_lnx_gfx_state->display, window->counter_xid, value);
          }
        }
        else if (evt.xclient.message_type == os_lnx_gfx_state->wakeup_atom)
        {
          OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_Wakeup);
          // Optional: copy data from event.xclient.data.l if needed
        }
      }break;

      //- dan: clipboard handling (SelectionRequest / PropertyNotify for getting data)
      case SelectionRequest:
      {
        XSelectionRequestEvent *req = &evt.xselectionrequest;
        XEvent response = {0};
        response.type = SelectionNotify;
        response.xselection.display = req->display;
        response.xselection.requestor = req->requestor;
        response.xselection.selection = req->selection;
        response.xselection.target = req->target;
        response.xselection.property = None; // Default to None (failure)
        response.xselection.time = req->time;

        // Only handle requests if we are the owner (using clipboard_window)
        if (req->owner == os_lnx_gfx_state->clipboard_window && req->selection == os_lnx_gfx_state->clipboard_atom)
        {
          if (req->target == os_lnx_gfx_state->targets_atom)
          {
             // Respond with the list of supported targets
             Atom supported_targets[] = { os_lnx_gfx_state->targets_atom, os_lnx_gfx_state->utf8_string_atom };
             XChangeProperty(d, req->requestor, req->property, XA_ATOM, 32, PropModeReplace, (unsigned char*)supported_targets, ArrayCount(supported_targets));
             response.xselection.property = req->property; // Indicate success
          }
          else if (req->target == os_lnx_gfx_state->utf8_string_atom)
          {
             // Respond with the actual clipboard data
             XChangeProperty(d, req->requestor, req->property, os_lnx_gfx_state->utf8_string_atom, 8, // 8 bits per char for UTF8
                           PropModeReplace, os_lnx_gfx_state->clipboard_buffer.str, os_lnx_gfx_state->clipboard_buffer.size);
             response.xselection.property = req->property; // Indicate success
          }
          // Add other target formats here if needed (e.g., TARGETS)
        }
        // If we are not the owner, or target is unsupported, property remains None (failure)

        XSendEvent(d, req->requestor, False, NoEventMask, &response);
        XFlush(d);
      } break;
      
      // This handles the notification that clipboard data is ready after we called XConvertSelection
      case PropertyNotify:
      { 
        if (evt.xproperty.state == PropertyNewValue && 
            evt.xproperty.window == os_lnx_gfx_state->clipboard_window && 
            evt.xproperty.atom == os_lnx_gfx_state->clipboard_target_prop_atom)
        {
           Atom actual_type;
           int actual_format;
           unsigned long nitems;
           unsigned long bytes_after;
           unsigned char *data = 0;
           int read_result = XGetWindowProperty(d, 
                                      os_lnx_gfx_state->clipboard_window, // Read from our dedicated window
                                      os_lnx_gfx_state->clipboard_target_prop_atom, // The property we specified
                                      0,      // Offset 0
                                      (~0L),  // Read the whole property (~0L is common practice for large size)
                                      True,   // Delete the property after reading
                                      AnyPropertyType, // Accept any type, but check actual_type later
                                      &actual_type, &actual_format, &nitems, &bytes_after, &data);

           if (read_result == Success && data != 0)
           {
             if (actual_type == os_lnx_gfx_state->utf8_string_atom && actual_format == 8)
             {
               arena_clear(os_lnx_gfx_state->clipboard_arena); // Use clipboard_arena
               os_lnx_gfx_state->last_retrieved_clipboard_text = push_str8_copy(os_lnx_gfx_state->clipboard_arena, str8(data, nitems)); // Use clipboard_arena
             }
             XFree(data);
           }
        }
      } break;
    }
  }
  return evts;
}

internal OS_Modifiers
os_get_modifiers(void)
{
  OS_Modifiers mods = 0;
  Display *d = os_lnx_gfx_state->display;
  if (d == 0) { return mods; } // Return 0 if display is invalid
  
  char keys_return[32]; // XQueryKeymap returns 32 bytes
  XQueryKeymap(d, keys_return);
  
  // Check Shift
  KeyCode kc_l = os_lnx_gfx_state->keycode_lshift;
  KeyCode kc_r = os_lnx_gfx_state->keycode_rshift;
  if ((kc_l != 0 && (keys_return[kc_l / 8] & (1 << (kc_l % 8)))) ||
      (kc_r != 0 && (keys_return[kc_r / 8] & (1 << (kc_r % 8)))))
  {
    mods |= OS_Modifier_Shift;
  }
  
  // Check Ctrl
  kc_l = os_lnx_gfx_state->keycode_lctrl;
  kc_r = os_lnx_gfx_state->keycode_rctrl;
  if ((kc_l != 0 && (keys_return[kc_l / 8] & (1 << (kc_l % 8)))) ||
      (kc_r != 0 && (keys_return[kc_r / 8] & (1 << (kc_r % 8)))))
  {
    mods |= OS_Modifier_Ctrl;
  }
  
  // Check Alt
  kc_l = os_lnx_gfx_state->keycode_lalt;
  kc_r = os_lnx_gfx_state->keycode_ralt;
  if ((kc_l != 0 && (keys_return[kc_l / 8] & (1 << (kc_l % 8)))) ||
      (kc_r != 0 && (keys_return[kc_r / 8] & (1 << (kc_r % 8)))))
  {
    mods |= OS_Modifier_Alt;
  }
  
  return mods;
}

internal B32
os_key_is_down(OS_Key key)
{
  if (key >= OS_Key_COUNT)
  {
      return 0;
  }
  
  // dan: query real-time key state instead of relying on cached event state
  Display *d = os_lnx_gfx_state->display;
  if (d == 0) { // Check if display is valid
      return 0;
  }
  KeyCode kc_left = 0, kc_right = 0;

  // Check if it's a modifier key and get specific left/right codes
  if (key == OS_Key_Shift)
  {
    kc_left = os_lnx_gfx_state->keycode_lshift;
    kc_right = os_lnx_gfx_state->keycode_rshift;
  }
  else if (key == OS_Key_Ctrl)
  {
    kc_left = os_lnx_gfx_state->keycode_lctrl;
    kc_right = os_lnx_gfx_state->keycode_rctrl;
  }
  else if (key == OS_Key_Alt)
  {
    kc_left = os_lnx_gfx_state->keycode_lalt;
    kc_right = os_lnx_gfx_state->keycode_ralt;
  }
  else
  {
    // Regular key, use the main table
    kc_left = os_lnx_gfx_state->keycode_from_oskey[key];
    // kc_right remains 0
  }
  
  // Check if at least one keycode is valid
  if (kc_left == 0 && kc_right == 0)
  {
    return 0; // Unmapped key
  }
  
  char keys_return[32]; // XQueryKeymap returns 32 bytes
  XQueryKeymap(d, keys_return);
  
  B32 is_down = 0;
  // Check the bit corresponding to the left KeyCode
  if (kc_left != 0 && (keys_return[kc_left / 8] & (1 << (kc_left % 8))))
  {
      is_down = 1;
  }
  // Check the bit corresponding to the right KeyCode if applicable
  if (!is_down && kc_right != 0 && (keys_return[kc_right / 8] & (1 << (kc_right % 8))))
  {
      is_down = 1;
  }
  
  return is_down;
}

internal Vec2F32
os_mouse_from_window(OS_Handle handle)
{
  Vec2F32 result = v2f32(0, 0);
  OS_LNX_Window *w = (OS_LNX_Window*)handle.u64[0];
  if (w)
  {
    Window root_return, child_return;
    int root_x_return, root_y_return, win_x_return, win_y_return;
    unsigned int mask_return;
    if (XQueryPointer(os_lnx_gfx_state->display, w->window, &root_return, &child_return,
                      &root_x_return, &root_y_return, &win_x_return, &win_y_return, &mask_return))
    {
       result.x = (F32)win_x_return;
       result.y = (F32)win_y_return;
    }
  }
  return result;
}

////////////////////////////////
//~ dan: @os_hooks Cursors (Implemented Per-OS)

internal void
os_set_cursor(OS_Cursor cursor)
{
  if (cursor < OS_Cursor_COUNT)
  {
     Cursor xcursor = os_lnx_gfx_state->cursors[cursor];
     if (xcursor != os_lnx_gfx_state->current_cursor)
     {
         os_lnx_gfx_state->current_cursor = xcursor;
         // Apply to all windows, or just the focused one? Apply to all for now.
         for(OS_LNX_Window *w = os_lnx_gfx_state->first_window; w != 0; w = w->next)
         {
            XDefineCursor(os_lnx_gfx_state->display, w->window, xcursor);
         }
         XFlush(os_lnx_gfx_state->display);
     }
  }
}

////////////////////////////////
//~ dan: @os_hooks Native User-Facing Graphical Messages (Implemented Per-OS)

internal void
os_graphical_message(B32 error, String8 title, String8 message)
{
  Temp scratch = scratch_begin(0, 0);
  String8List cmd_line = {0};

  str8_list_push(scratch.arena, &cmd_line, str8_lit("zenity"));
  if(error)
  {
    str8_list_push(scratch.arena, &cmd_line, str8_lit("--error"));
  }
  else
  {
    str8_list_push(scratch.arena, &cmd_line, str8_lit("--info"));
  }

  // Format title and message arguments safely for the command line
  String8 title_arg = push_str8f(scratch.arena, "--title=\"%S\"", title);
  String8 text_arg = push_str8f(scratch.arena, "--text=\"%S\"", message);

  str8_list_push(scratch.arena, &cmd_line, title_arg);
  str8_list_push(scratch.arena, &cmd_line, text_arg);

  // Launch the process
  OS_ProcessLaunchParams params = {0};
  params.cmd_line = cmd_line;
  params.inherit_env = 1;
  params.consoleless = 1; // Don't need a console for zenity

  OS_Handle proc_handle = os_process_launch(&params);

  // We don't need to wait for the dialog, detach immediately.
  if (!os_handle_match(proc_handle, os_handle_zero()))
  {
    os_process_detach(proc_handle);
  }
  scratch_end(scratch);
}

////////////////////////////////
//~ dan: @os_hooks Shell Operations

internal void
os_show_in_filesystem_ui(String8 path)
{
  Temp scratch = scratch_begin(0, 0);
  String8 path_copy = push_str8_copy(scratch.arena, path);

  // Get the parent directory of the path
  String8 dir_path = str8_chop_last_slash(path_copy);
  if (dir_path.size == 0) // Handle root or relative path in cwd
  {
    dir_path = str8_lit(".");
  }

  // Construct the command line: xdg-open <directory>
  String8List cmd_line = {0};
  str8_list_push(scratch.arena, &cmd_line, str8_lit("xdg-open"));
  str8_list_push(scratch.arena, &cmd_line, dir_path);

  // Launch the process
  OS_ProcessLaunchParams params = {0};
  params.cmd_line = cmd_line;
  params.inherit_env = 1;
  params.consoleless = 1; // Don't need a console for xdg-open

  OS_Handle proc_handle = os_process_launch(&params);

  // We don't need to wait for xdg-open, detach immediately.
  if (!os_handle_match(proc_handle, os_handle_zero()))
  {
    os_process_detach(proc_handle);
  }
  scratch_end(scratch);
}

internal void
os_open_in_browser(String8 url)
{
  Temp scratch = scratch_begin(0, 0);
  // Construct the command line: xdg-open <url>
  String8List cmd_line = {0};
  str8_list_push(scratch.arena, &cmd_line, str8_lit("xdg-open"));
  str8_list_push(scratch.arena, &cmd_line, url); // Pass URL directly

  // Launch the process
  OS_ProcessLaunchParams params = {0};
  params.cmd_line = cmd_line;
  params.inherit_env = 1;
  params.consoleless = 1; // Don't need a console for xdg-open

  OS_Handle proc_handle = os_process_launch(&params);

  // We don't need to wait for xdg-open, detach immediately.
  if (!os_handle_match(proc_handle, os_handle_zero()))
  {
    os_process_detach(proc_handle);
  }
  scratch_end(scratch);
}

