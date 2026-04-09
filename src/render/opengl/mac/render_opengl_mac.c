// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

typedef void (*DEBUGPROC)(GLenum source,
          GLenum type,
          GLuint id,
          GLenum severity,
          GLsizei length,
          const GLchar *message,
          const void *userParam);

internal void
glDebugMessageCallbackStub(
  DEBUGPROC callback,
  const void * userParam
) {}

/*
 ** Attribute names for [NSOpenGLPixelFormat initWithAttributes]
 */
enum {
    NSOpenGLPFAAllRenderers           =   1,	/* choose from all available renderers          */
    NSOpenGLPFATripleBuffer           =   3,	/* choose a triple buffered pixel format        */
    NSOpenGLPFADoubleBuffer           =   5,	/* choose a double buffered pixel format        */
    NSOpenGLPFAAuxBuffers             =   7,	/* number of aux buffers                        */
    NSOpenGLPFAColorSize              =   8,	/* number of color buffer bits                  */
    NSOpenGLPFAAlphaSize              =  11,	/* number of alpha component bits               */
    NSOpenGLPFADepthSize              =  12,	/* number of depth buffer bits                  */
    NSOpenGLPFAStencilSize            =  13,	/* number of stencil buffer bits                */
    NSOpenGLPFAAccumSize              =  14,	/* number of accum buffer bits                  */
    NSOpenGLPFAMinimumPolicy          =  51,	/* never choose smaller buffers than requested  */
    NSOpenGLPFAMaximumPolicy          =  52,	/* choose largest buffers of type requested     */
    NSOpenGLPFASampleBuffers          =  55,	/* number of multi sample buffers               */
    NSOpenGLPFASamples                =  56,	/* number of samples per multi sample buffer    */
    NSOpenGLPFAAuxDepthStencil        =  57,	/* each aux buffer has its own depth stencil    */
    NSOpenGLPFAColorFloat             =  58,	/* color buffers store floating point pixels    */
    NSOpenGLPFAMultisample            =  59,  /* choose multisampling                         */
    NSOpenGLPFASupersample            =  60,  /* choose supersampling                         */
    NSOpenGLPFASampleAlpha            =  61,  /* request alpha filtering                      */
    NSOpenGLPFARendererID             =  70,	/* request renderer by ID                       */
    NSOpenGLPFANoRecovery             =  72,	/* disable all failure recovery systems         */
    NSOpenGLPFAAccelerated            =  73,	/* choose a hardware accelerated renderer       */
    NSOpenGLPFAClosestPolicy          =  74,	/* choose the closest color buffer to request   */
    NSOpenGLPFABackingStore           =  76,	/* back buffer contents are valid after swap    */
    NSOpenGLPFAScreenMask             =  84,	/* bit mask of supported physical screens       */
    NSOpenGLPFAAllowOfflineRenderers  =  96,  /* allow use of offline renderers               */
    NSOpenGLPFAAcceleratedCompute     =  97,	/* choose a hardware accelerated compute device */
    NSOpenGLPFAOpenGLProfile          =  99,  /* specify an OpenGL Profile to use             */
    NSOpenGLPFAVirtualScreenCount     = 128,	/* number of virtual screens in this format     */
    
    NSOpenGLPFAStereo                 =   6,
    NSOpenGLPFAOffScreen              =  53,
    NSOpenGLPFAFullScreen             =  54,
    NSOpenGLPFASingleRenderer         =  71,
    NSOpenGLPFARobust                 =  75,
    NSOpenGLPFAMPSafe                 =  78,
    NSOpenGLPFAWindow                 =  80,
    NSOpenGLPFAMultiScreen            =  81,
    NSOpenGLPFACompliant              =  83,
    NSOpenGLPFAPixelBuffer            =  90,
    NSOpenGLPFARemotePixelBuffer      =  91,
};

/* NSOpenGLPFAOpenGLProfile values */
enum {
    NSOpenGLProfileVersionLegacy      = 0x1000,   /* choose a Legacy/Pre-OpenGL 3.0 Implementation */
    NSOpenGLProfileVersion3_2Core     = 0x3200,   /* choose an OpenGL 3.2 Core Implementation      */
    NSOpenGLProfileVersion4_1Core     = 0x4100    /* choose an OpenGL 4.1 Core Implementation      */
};

internal VoidProc *
r_ogl_os_load_procedure(char *name)
{
  if(strcmp(name, "glDebugMessageCallback") == 0)
  {
    return (VoidProc *)glDebugMessageCallbackStub;
  }
  return (VoidProc *)dlsym(RTLD_DEFAULT, name);
}

internal void
r_ogl_os_init(CmdLine *cmdln)
{
  U32 attr[] = {
    NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAColorSize, 24,
    NSOpenGLPFAAlphaSize,     8,
    NSOpenGLPFADepthSize,     24,
    NSOpenGLPFASupersample,
    NSOpenGLPFASampleBuffers, 1,
    NSOpenGLPFASamples, 4,
    NSOpenGLPFAMultisample,
    0
  };
  
  id format = msg1(id, msg(id, cls("NSOpenGLPixelFormat"), "alloc"), "initWithAttributes:", U32 *, attr);
  
  CGRect rect = {{0,0}, {800,600}};
  id view = msg2(id, msg(id, cls("NSOpenGLView"), "alloc"), "initWithFrame:pixelFormat:", CGRect, rect, id, format);
  r_ogl_mac_ctx = msg(id, view, "openGLContext");

  if(r_ogl_mac_ctx == 0)
  {
    printf("gl context is nil\n");
    os_abort(1);
  }

  msg(void, r_ogl_mac_ctx, "makeCurrentContext");
}

internal R_Handle
r_ogl_os_window_equip(OS_Handle window)
{
  // dummy
  R_Handle result = {0};
  return result;
}

internal void
r_ogl_os_window_unequip(OS_Handle os, R_Handle r)
{
  // dummy
}

internal void
r_ogl_os_select_window(OS_Handle window, R_Handle r)
{
  if(os_handle_match(window, os_handle_zero())) {return;}
  OS_MAC_Window *w = (OS_MAC_Window *)window.u64[0];

  id view = msg(id, w->win, "contentView");

  if (msg(Class, view, "class") != cls("NSOpenGLView")) {
    CGRect rect = msg(CGRect, view, "frame");

    id format = msg(id, cls("NSOpenGLView"), "defaultPixelFormat");
    view = msg2(id, msg(id, cls("NSOpenGLView"), "alloc"), "initWithFrame:pixelFormat:", CGRect, rect, id, format);
    msg1(void, w->win, "setContentView:", id, view);

    msg(void, view, "clearGLContext");
    msg1(void, view, "setOpenGLContext:", id, r_ogl_mac_ctx);

    NSInteger redraw_during_view_resize = 2;
    NSInteger contents_placement_top_left = 11;
    U32 width_sizable = 1 << 1;
    U32 height_sizable = 1 << 4;
    U32 autoresizing_mask = width_sizable | height_sizable;

    id layer = msg(id, view, "layer");

    msg1(void, view, "setLayerContentsPlacement:", NSInteger, contents_placement_top_left);
    msg1(void, view, "setLayerContentsRedrawPolicy:", NSInteger, redraw_during_view_resize);

    msg1(void, layer, "setAutoresizingMask:", U32, autoresizing_mask);
    msg1(void, layer, "setNeedsDisplayOnBoundsChange:", BOOL, YES);
  }

  msg1(void, r_ogl_mac_ctx, "setView:", id, view);
  msg(void, r_ogl_mac_ctx, "makeCurrentContext");
}

internal void
r_ogl_os_window_swap(OS_Handle os, R_Handle r)
{
  glFlush();
  msg(void, r_ogl_mac_ctx, "flushBuffer");
}
