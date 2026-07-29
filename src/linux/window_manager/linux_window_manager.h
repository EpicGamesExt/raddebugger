// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef LINUX_WINDOW_MANAGER_H
#define LINUX_WINDOW_MANAGER_H

////////////////////////////////
//~ rjf: Includes

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/extensions/sync.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

////////////////////////////////
//~ rjf: Wayland Client Bindings
//
// Hand-declared subset of libwayland-client / libwayland-egl / libwayland-cursor
// / libxkbcommon / xdg-shell / xdg-decoration - just the parts the Wayland backend
// uses, so we do not pull in the system headers or generated protocol code. The
// libraries are still linked; the xdg protocol wire-data is defined in
// wayland_window_manager.c. `global` is undefined here because wl_registry_listener
// has a member named `global`.

#pragma push_macro("global")
#undef global

typedef int32_t wl_fixed_t;

struct wl_message { const char *name; const char *signature; const struct wl_interface **types; };
struct wl_interface { const char *name; int version; int method_count; const struct wl_message *methods; int event_count; const struct wl_message *events; };
struct wl_array { size_t size; size_t alloc; void *data; };

struct wl_proxy; struct wl_display; struct wl_registry; struct wl_compositor;
struct wl_surface; struct wl_seat; struct wl_pointer; struct wl_keyboard;
struct wl_shm; struct wl_region; struct wl_buffer; struct wl_output; struct wl_callback;
struct wl_data_device_manager; struct wl_data_device; struct wl_data_source; struct wl_data_offer;

extern struct wl_proxy *wl_proxy_marshal_flags(struct wl_proxy *proxy, uint32_t opcode, const struct wl_interface *interface, uint32_t version, uint32_t flags, ...);
extern int wl_proxy_add_listener(struct wl_proxy *proxy, void (**implementation)(void), void *data);
extern void *wl_proxy_get_user_data(struct wl_proxy *proxy);
extern uint32_t wl_proxy_get_version(struct wl_proxy *proxy);
extern struct wl_display *wl_display_connect(const char *name);
extern void wl_display_disconnect(struct wl_display *display);
extern int wl_display_roundtrip(struct wl_display *display);
extern int wl_display_dispatch_pending(struct wl_display *display);
extern int wl_display_flush(struct wl_display *display);
extern int wl_display_get_fd(struct wl_display *display);
extern int wl_display_prepare_read(struct wl_display *display);
extern int wl_display_read_events(struct wl_display *display);
extern void wl_display_cancel_read(struct wl_display *display);

#define WL_MARSHAL_FLAG_DESTROY (1 << 0)

static inline double wl_fixed_to_double(wl_fixed_t f) { return f / 256.0; }

extern const struct wl_interface wl_registry_interface;
extern const struct wl_interface wl_compositor_interface;
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface wl_seat_interface;
extern const struct wl_interface wl_pointer_interface;
extern const struct wl_interface wl_keyboard_interface;
extern const struct wl_interface wl_region_interface;
extern const struct wl_interface wl_shm_interface;
extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_data_device_manager_interface;
extern const struct wl_interface wl_data_device_interface;
extern const struct wl_interface wl_data_source_interface;
extern const struct wl_interface wl_data_offer_interface;

#define WL_DISPLAY_GET_REGISTRY 1
#define WL_REGISTRY_BIND 0
#define WL_COMPOSITOR_CREATE_SURFACE 0
#define WL_COMPOSITOR_CREATE_REGION 1
#define WL_SURFACE_DESTROY 0
#define WL_SURFACE_ATTACH 1
#define WL_SURFACE_DAMAGE 2
#define WL_SURFACE_SET_OPAQUE_REGION 4
#define WL_SURFACE_COMMIT 6
#define WL_REGION_DESTROY 0
#define WL_REGION_ADD 1
#define WL_SEAT_GET_POINTER 0
#define WL_SEAT_GET_KEYBOARD 1
#define WL_POINTER_SET_CURSOR 0
#define WL_DATA_DEVICE_MANAGER_CREATE_DATA_SOURCE 0
#define WL_DATA_DEVICE_MANAGER_GET_DATA_DEVICE 1
#define WL_DATA_DEVICE_SET_SELECTION 1
#define WL_DATA_SOURCE_OFFER 0
#define WL_DATA_SOURCE_DESTROY 1
#define WL_DATA_OFFER_RECEIVE 1
#define WL_DATA_OFFER_DESTROY 2

enum { WL_SEAT_CAPABILITY_POINTER = 1, WL_SEAT_CAPABILITY_KEYBOARD = 2 };
enum { WL_POINTER_BUTTON_STATE_PRESSED = 1 };
enum { WL_POINTER_AXIS_HORIZONTAL_SCROLL = 1 };
enum { WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 = 1 };
enum { WL_KEYBOARD_KEY_STATE_PRESSED = 1 };

struct wl_registry_listener
{
  void (*global)(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version);
  void (*global_remove)(void *data, struct wl_registry *wl_registry, uint32_t name);
};
struct wl_seat_listener
{
  void (*capabilities)(void *data, struct wl_seat *wl_seat, uint32_t capabilities);
  void (*name)(void *data, struct wl_seat *wl_seat, const char *name);
};
struct wl_pointer_listener
{
  void (*enter)(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
  void (*leave)(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface);
  void (*motion)(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y);
  void (*button)(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
  void (*axis)(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
  void (*frame)(void *data, struct wl_pointer *wl_pointer);
  void (*axis_source)(void *data, struct wl_pointer *wl_pointer, uint32_t axis_source);
  void (*axis_stop)(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis);
  void (*axis_discrete)(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete);
  void (*axis_value120)(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t value120);
  void (*axis_relative_direction)(void *data, struct wl_pointer *wl_pointer, uint32_t axis, uint32_t direction);
};
struct wl_keyboard_listener
{
  void (*keymap)(void *data, struct wl_keyboard *wl_keyboard, uint32_t format, int32_t fd, uint32_t size);
  void (*enter)(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys);
  void (*leave)(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface);
  void (*key)(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
  void (*modifiers)(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
  void (*repeat_info)(void *data, struct wl_keyboard *wl_keyboard, int32_t rate, int32_t delay);
};

static inline struct wl_registry *wl_display_get_registry(struct wl_display *wl_display)
{ return (struct wl_registry *)wl_proxy_marshal_flags((struct wl_proxy *)wl_display, WL_DISPLAY_GET_REGISTRY, &wl_registry_interface, wl_proxy_get_version((struct wl_proxy *)wl_display), 0, NULL); }
static inline void *wl_registry_bind(struct wl_registry *wl_registry, uint32_t name, const struct wl_interface *interface, uint32_t version)
{ return (void *)wl_proxy_marshal_flags((struct wl_proxy *)wl_registry, WL_REGISTRY_BIND, interface, version, 0, name, interface->name, version, NULL); }
static inline int wl_registry_add_listener(struct wl_registry *wl_registry, const struct wl_registry_listener *listener, void *data)
{ return wl_proxy_add_listener((struct wl_proxy *)wl_registry, (void (**)(void))listener, data); }
static inline struct wl_surface *wl_compositor_create_surface(struct wl_compositor *wl_compositor)
{ return (struct wl_surface *)wl_proxy_marshal_flags((struct wl_proxy *)wl_compositor, WL_COMPOSITOR_CREATE_SURFACE, &wl_surface_interface, wl_proxy_get_version((struct wl_proxy *)wl_compositor), 0, NULL); }
static inline struct wl_region *wl_compositor_create_region(struct wl_compositor *wl_compositor)
{ return (struct wl_region *)wl_proxy_marshal_flags((struct wl_proxy *)wl_compositor, WL_COMPOSITOR_CREATE_REGION, &wl_region_interface, wl_proxy_get_version((struct wl_proxy *)wl_compositor), 0, NULL); }
static inline void wl_surface_destroy(struct wl_surface *wl_surface)
{ wl_proxy_marshal_flags((struct wl_proxy *)wl_surface, WL_SURFACE_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *)wl_surface), WL_MARSHAL_FLAG_DESTROY); }
static inline void wl_surface_attach(struct wl_surface *wl_surface, struct wl_buffer *buffer, int32_t x, int32_t y)
{ wl_proxy_marshal_flags((struct wl_proxy *)wl_surface, WL_SURFACE_ATTACH, NULL, wl_proxy_get_version((struct wl_proxy *)wl_surface), 0, buffer, x, y); }
static inline void wl_surface_damage(struct wl_surface *wl_surface, int32_t x, int32_t y, int32_t width, int32_t height)
{ wl_proxy_marshal_flags((struct wl_proxy *)wl_surface, WL_SURFACE_DAMAGE, NULL, wl_proxy_get_version((struct wl_proxy *)wl_surface), 0, x, y, width, height); }
static inline void wl_surface_set_opaque_region(struct wl_surface *wl_surface, struct wl_region *region)
{ wl_proxy_marshal_flags((struct wl_proxy *)wl_surface, WL_SURFACE_SET_OPAQUE_REGION, NULL, wl_proxy_get_version((struct wl_proxy *)wl_surface), 0, region); }
static inline void wl_surface_commit(struct wl_surface *wl_surface)
{ wl_proxy_marshal_flags((struct wl_proxy *)wl_surface, WL_SURFACE_COMMIT, NULL, wl_proxy_get_version((struct wl_proxy *)wl_surface), 0); }
static inline void wl_region_add(struct wl_region *wl_region, int32_t x, int32_t y, int32_t width, int32_t height)
{ wl_proxy_marshal_flags((struct wl_proxy *)wl_region, WL_REGION_ADD, NULL, wl_proxy_get_version((struct wl_proxy *)wl_region), 0, x, y, width, height); }
static inline void wl_region_destroy(struct wl_region *wl_region)
{ wl_proxy_marshal_flags((struct wl_proxy *)wl_region, WL_REGION_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *)wl_region), WL_MARSHAL_FLAG_DESTROY); }
static inline int wl_seat_add_listener(struct wl_seat *wl_seat, const struct wl_seat_listener *listener, void *data)
{ return wl_proxy_add_listener((struct wl_proxy *)wl_seat, (void (**)(void))listener, data); }
static inline struct wl_pointer *wl_seat_get_pointer(struct wl_seat *wl_seat)
{ return (struct wl_pointer *)wl_proxy_marshal_flags((struct wl_proxy *)wl_seat, WL_SEAT_GET_POINTER, &wl_pointer_interface, wl_proxy_get_version((struct wl_proxy *)wl_seat), 0, NULL); }
static inline struct wl_keyboard *wl_seat_get_keyboard(struct wl_seat *wl_seat)
{ return (struct wl_keyboard *)wl_proxy_marshal_flags((struct wl_proxy *)wl_seat, WL_SEAT_GET_KEYBOARD, &wl_keyboard_interface, wl_proxy_get_version((struct wl_proxy *)wl_seat), 0, NULL); }
static inline int wl_pointer_add_listener(struct wl_pointer *wl_pointer, const struct wl_pointer_listener *listener, void *data)
{ return wl_proxy_add_listener((struct wl_proxy *)wl_pointer, (void (**)(void))listener, data); }
static inline void wl_pointer_set_cursor(struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, int32_t hotspot_x, int32_t hotspot_y)
{ wl_proxy_marshal_flags((struct wl_proxy *)wl_pointer, WL_POINTER_SET_CURSOR, NULL, wl_proxy_get_version((struct wl_proxy *)wl_pointer), 0, serial, surface, hotspot_x, hotspot_y); }
static inline int wl_keyboard_add_listener(struct wl_keyboard *wl_keyboard, const struct wl_keyboard_listener *listener, void *data)
{ return wl_proxy_add_listener((struct wl_proxy *)wl_keyboard, (void (**)(void))listener, data); }

// rjf: wl_data_device_manager & friends - the core-protocol clipboard ("selection").
// The v3 drag & drop events are declared so the listener tables stay the right
// size when the manager is bound at version 3; we never act on them.
struct wl_data_offer_listener
{
  void (*offer)(void *data, struct wl_data_offer *wl_data_offer, const char *mime_type);
  void (*source_actions)(void *data, struct wl_data_offer *wl_data_offer, uint32_t source_actions);
  void (*action)(void *data, struct wl_data_offer *wl_data_offer, uint32_t dnd_action);
};
struct wl_data_source_listener
{
  void (*target)(void *data, struct wl_data_source *wl_data_source, const char *mime_type);
  void (*send)(void *data, struct wl_data_source *wl_data_source, const char *mime_type, int32_t fd);
  void (*cancelled)(void *data, struct wl_data_source *wl_data_source);
  void (*dnd_drop_performed)(void *data, struct wl_data_source *wl_data_source);
  void (*dnd_finished)(void *data, struct wl_data_source *wl_data_source);
  void (*action)(void *data, struct wl_data_source *wl_data_source, uint32_t dnd_action);
};
struct wl_data_device_listener
{
  void (*data_offer)(void *data, struct wl_data_device *wl_data_device, struct wl_data_offer *id);
  void (*enter)(void *data, struct wl_data_device *wl_data_device, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y, struct wl_data_offer *id);
  void (*leave)(void *data, struct wl_data_device *wl_data_device);
  void (*motion)(void *data, struct wl_data_device *wl_data_device, uint32_t time, wl_fixed_t x, wl_fixed_t y);
  void (*drop)(void *data, struct wl_data_device *wl_data_device);
  void (*selection)(void *data, struct wl_data_device *wl_data_device, struct wl_data_offer *id);
};

static inline struct wl_data_source *wl_data_device_manager_create_data_source(struct wl_data_device_manager *manager)
{ return (struct wl_data_source *)wl_proxy_marshal_flags((struct wl_proxy *)manager, WL_DATA_DEVICE_MANAGER_CREATE_DATA_SOURCE, &wl_data_source_interface, wl_proxy_get_version((struct wl_proxy *)manager), 0, NULL); }
static inline struct wl_data_device *wl_data_device_manager_get_data_device(struct wl_data_device_manager *manager, struct wl_seat *seat)
{ return (struct wl_data_device *)wl_proxy_marshal_flags((struct wl_proxy *)manager, WL_DATA_DEVICE_MANAGER_GET_DATA_DEVICE, &wl_data_device_interface, wl_proxy_get_version((struct wl_proxy *)manager), 0, NULL, seat); }
static inline int wl_data_device_add_listener(struct wl_data_device *device, const struct wl_data_device_listener *listener, void *data)
{ return wl_proxy_add_listener((struct wl_proxy *)device, (void (**)(void))listener, data); }
static inline void wl_data_device_set_selection(struct wl_data_device *device, struct wl_data_source *source, uint32_t serial)
{ wl_proxy_marshal_flags((struct wl_proxy *)device, WL_DATA_DEVICE_SET_SELECTION, NULL, wl_proxy_get_version((struct wl_proxy *)device), 0, source, serial); }
static inline int wl_data_source_add_listener(struct wl_data_source *source, const struct wl_data_source_listener *listener, void *data)
{ return wl_proxy_add_listener((struct wl_proxy *)source, (void (**)(void))listener, data); }
static inline void wl_data_source_offer(struct wl_data_source *source, const char *mime_type)
{ wl_proxy_marshal_flags((struct wl_proxy *)source, WL_DATA_SOURCE_OFFER, NULL, wl_proxy_get_version((struct wl_proxy *)source), 0, mime_type); }
static inline void wl_data_source_destroy(struct wl_data_source *source)
{ wl_proxy_marshal_flags((struct wl_proxy *)source, WL_DATA_SOURCE_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *)source), WL_MARSHAL_FLAG_DESTROY); }
static inline int wl_data_offer_add_listener(struct wl_data_offer *offer, const struct wl_data_offer_listener *listener, void *data)
{ return wl_proxy_add_listener((struct wl_proxy *)offer, (void (**)(void))listener, data); }
static inline void wl_data_offer_receive(struct wl_data_offer *offer, const char *mime_type, int32_t fd)
{ wl_proxy_marshal_flags((struct wl_proxy *)offer, WL_DATA_OFFER_RECEIVE, NULL, wl_proxy_get_version((struct wl_proxy *)offer), 0, mime_type, fd); }
static inline void wl_data_offer_destroy(struct wl_data_offer *offer)
{ wl_proxy_marshal_flags((struct wl_proxy *)offer, WL_DATA_OFFER_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *)offer), WL_MARSHAL_FLAG_DESTROY); }

// rjf: libwayland-egl
struct wl_egl_window;
extern struct wl_egl_window *wl_egl_window_create(struct wl_surface *surface, int width, int height);
extern void wl_egl_window_destroy(struct wl_egl_window *egl_window);
extern void wl_egl_window_resize(struct wl_egl_window *egl_window, int width, int height, int dx, int dy);

// rjf: libwayland-cursor
struct wl_cursor_theme;
struct wl_cursor_image { uint32_t width; uint32_t height; uint32_t hotspot_x; uint32_t hotspot_y; uint32_t delay; };
struct wl_cursor { unsigned int image_count; struct wl_cursor_image **images; char *name; };
extern struct wl_cursor_theme *wl_cursor_theme_load(const char *name, int size, struct wl_shm *shm);
extern struct wl_cursor *wl_cursor_theme_get_cursor(struct wl_cursor_theme *theme, const char *name);
extern struct wl_buffer *wl_cursor_image_get_buffer(struct wl_cursor_image *image);

// rjf: libxkbcommon
typedef uint32_t xkb_keycode_t;
typedef uint32_t xkb_keysym_t;
typedef uint32_t xkb_mod_mask_t;
typedef uint32_t xkb_layout_index_t;
struct xkb_context; struct xkb_keymap; struct xkb_state;
enum { XKB_CONTEXT_NO_FLAGS = 0 };
enum { XKB_KEYMAP_FORMAT_TEXT_V1 = 1 };
enum { XKB_KEYMAP_COMPILE_NO_FLAGS = 0 };
enum { XKB_STATE_MODS_EFFECTIVE = (1 << 3) };
#define XKB_MOD_NAME_SHIFT "Shift"
#define XKB_MOD_NAME_CTRL  "Control"
#define XKB_MOD_NAME_ALT   "Mod1"
extern struct xkb_context *xkb_context_new(int flags);
extern struct xkb_keymap *xkb_keymap_new_from_string(struct xkb_context *context, const char *string, int format, int flags);
extern void xkb_keymap_unref(struct xkb_keymap *keymap);
extern int xkb_keymap_key_repeats(struct xkb_keymap *keymap, xkb_keycode_t key);
extern struct xkb_state *xkb_state_new(struct xkb_keymap *keymap);
extern void xkb_state_unref(struct xkb_state *state);
extern xkb_keysym_t xkb_state_key_get_one_sym(struct xkb_state *state, xkb_keycode_t key);
extern int xkb_state_key_get_utf8(struct xkb_state *state, xkb_keycode_t key, char *buffer, size_t size);
extern int xkb_state_update_mask(struct xkb_state *state, xkb_mod_mask_t depressed_mods, xkb_mod_mask_t latched_mods, xkb_mod_mask_t locked_mods, xkb_layout_index_t depressed_layout, xkb_layout_index_t latched_layout, xkb_layout_index_t locked_layout);
extern int xkb_state_mod_name_is_active(struct xkb_state *state, const char *name, int type);

// rjf: xdg-shell (wire-data defined in wayland_window_manager.c)
struct xdg_wm_base; struct xdg_surface; struct xdg_toplevel;
extern const struct wl_interface xdg_wm_base_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;
#define XDG_WM_BASE_GET_XDG_SURFACE 2
#define XDG_WM_BASE_PONG 3
#define XDG_SURFACE_DESTROY 0
#define XDG_SURFACE_GET_TOPLEVEL 1
#define XDG_SURFACE_ACK_CONFIGURE 4
#define XDG_TOPLEVEL_DESTROY 0
#define XDG_TOPLEVEL_SET_TITLE 2
#define XDG_TOPLEVEL_SET_APP_ID 3
#define XDG_TOPLEVEL_MOVE 5
#define XDG_TOPLEVEL_RESIZE 6
#define XDG_TOPLEVEL_SET_MAXIMIZED 9
#define XDG_TOPLEVEL_UNSET_MAXIMIZED 10
#define XDG_TOPLEVEL_SET_FULLSCREEN 11
#define XDG_TOPLEVEL_UNSET_FULLSCREEN 12
#define XDG_TOPLEVEL_SET_MINIMIZED 13
enum xdg_toplevel_resize_edge
{
  XDG_TOPLEVEL_RESIZE_EDGE_NONE = 0,
  XDG_TOPLEVEL_RESIZE_EDGE_TOP = 1,
  XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM = 2,
  XDG_TOPLEVEL_RESIZE_EDGE_LEFT = 4,
  XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT = 5,
  XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT = 6,
  XDG_TOPLEVEL_RESIZE_EDGE_RIGHT = 8,
  XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT = 9,
  XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT = 10,
};
enum xdg_toplevel_state
{
  XDG_TOPLEVEL_STATE_MAXIMIZED = 1,
  XDG_TOPLEVEL_STATE_FULLSCREEN = 2,
  XDG_TOPLEVEL_STATE_ACTIVATED = 4,
};
struct xdg_wm_base_listener { void (*ping)(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial); };
struct xdg_surface_listener { void (*configure)(void *data, struct xdg_surface *xdg_surface, uint32_t serial); };
struct xdg_toplevel_listener
{
  void (*configure)(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states);
  void (*close)(void *data, struct xdg_toplevel *xdg_toplevel);
  void (*configure_bounds)(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height);
  void (*wm_capabilities)(void *data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities);
};
static inline int xdg_wm_base_add_listener(struct xdg_wm_base *xdg_wm_base, const struct xdg_wm_base_listener *listener, void *data)
{ return wl_proxy_add_listener((struct wl_proxy *)xdg_wm_base, (void (**)(void))listener, data); }
static inline struct xdg_surface *xdg_wm_base_get_xdg_surface(struct xdg_wm_base *xdg_wm_base, struct wl_surface *surface)
{ return (struct xdg_surface *)wl_proxy_marshal_flags((struct wl_proxy *)xdg_wm_base, XDG_WM_BASE_GET_XDG_SURFACE, &xdg_surface_interface, wl_proxy_get_version((struct wl_proxy *)xdg_wm_base), 0, NULL, surface); }
static inline void xdg_wm_base_pong(struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_wm_base, XDG_WM_BASE_PONG, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_wm_base), 0, serial); }
static inline int xdg_surface_add_listener(struct xdg_surface *xdg_surface, const struct xdg_surface_listener *listener, void *data)
{ return wl_proxy_add_listener((struct wl_proxy *)xdg_surface, (void (**)(void))listener, data); }
static inline struct xdg_toplevel *xdg_surface_get_toplevel(struct xdg_surface *xdg_surface)
{ return (struct xdg_toplevel *)wl_proxy_marshal_flags((struct wl_proxy *)xdg_surface, XDG_SURFACE_GET_TOPLEVEL, &xdg_toplevel_interface, wl_proxy_get_version((struct wl_proxy *)xdg_surface), 0, NULL); }
static inline void xdg_surface_ack_configure(struct xdg_surface *xdg_surface, uint32_t serial)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_surface, XDG_SURFACE_ACK_CONFIGURE, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_surface), 0, serial); }
static inline void xdg_surface_destroy(struct xdg_surface *xdg_surface)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_surface, XDG_SURFACE_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_surface), WL_MARSHAL_FLAG_DESTROY); }
static inline int xdg_toplevel_add_listener(struct xdg_toplevel *xdg_toplevel, const struct xdg_toplevel_listener *listener, void *data)
{ return wl_proxy_add_listener((struct wl_proxy *)xdg_toplevel, (void (**)(void))listener, data); }
static inline void xdg_toplevel_destroy(struct xdg_toplevel *xdg_toplevel)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_toplevel, XDG_TOPLEVEL_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_toplevel), WL_MARSHAL_FLAG_DESTROY); }
static inline void xdg_toplevel_set_title(struct xdg_toplevel *xdg_toplevel, const char *title)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_toplevel, XDG_TOPLEVEL_SET_TITLE, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_toplevel), 0, title); }
static inline void xdg_toplevel_set_app_id(struct xdg_toplevel *xdg_toplevel, const char *app_id)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_toplevel, XDG_TOPLEVEL_SET_APP_ID, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_toplevel), 0, app_id); }
static inline void xdg_toplevel_move(struct xdg_toplevel *xdg_toplevel, struct wl_seat *seat, uint32_t serial)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_toplevel, XDG_TOPLEVEL_MOVE, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_toplevel), 0, seat, serial); }
static inline void xdg_toplevel_resize(struct xdg_toplevel *xdg_toplevel, struct wl_seat *seat, uint32_t serial, uint32_t edges)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_toplevel, XDG_TOPLEVEL_RESIZE, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_toplevel), 0, seat, serial, edges); }
static inline void xdg_toplevel_set_maximized(struct xdg_toplevel *xdg_toplevel)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_toplevel, XDG_TOPLEVEL_SET_MAXIMIZED, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_toplevel), 0); }
static inline void xdg_toplevel_unset_maximized(struct xdg_toplevel *xdg_toplevel)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_toplevel, XDG_TOPLEVEL_UNSET_MAXIMIZED, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_toplevel), 0); }
static inline void xdg_toplevel_set_fullscreen(struct xdg_toplevel *xdg_toplevel, struct wl_output *output)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_toplevel, XDG_TOPLEVEL_SET_FULLSCREEN, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_toplevel), 0, output); }
static inline void xdg_toplevel_unset_fullscreen(struct xdg_toplevel *xdg_toplevel)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_toplevel, XDG_TOPLEVEL_UNSET_FULLSCREEN, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_toplevel), 0); }
static inline void xdg_toplevel_set_minimized(struct xdg_toplevel *xdg_toplevel)
{ wl_proxy_marshal_flags((struct wl_proxy *)xdg_toplevel, XDG_TOPLEVEL_SET_MINIMIZED, NULL, wl_proxy_get_version((struct wl_proxy *)xdg_toplevel), 0); }

// rjf: xdg-decoration (wire-data defined in wayland_window_manager.c)
struct zxdg_decoration_manager_v1; struct zxdg_toplevel_decoration_v1;
extern const struct wl_interface zxdg_decoration_manager_v1_interface;
extern const struct wl_interface zxdg_toplevel_decoration_v1_interface;
#define ZXDG_DECORATION_MANAGER_V1_GET_TOPLEVEL_DECORATION 1
#define ZXDG_TOPLEVEL_DECORATION_V1_DESTROY 0
#define ZXDG_TOPLEVEL_DECORATION_V1_SET_MODE 1
enum { ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE = 1 };
static inline struct zxdg_toplevel_decoration_v1 *zxdg_decoration_manager_v1_get_toplevel_decoration(struct zxdg_decoration_manager_v1 *manager, struct xdg_toplevel *toplevel)
{ return (struct zxdg_toplevel_decoration_v1 *)wl_proxy_marshal_flags((struct wl_proxy *)manager, ZXDG_DECORATION_MANAGER_V1_GET_TOPLEVEL_DECORATION, &zxdg_toplevel_decoration_v1_interface, wl_proxy_get_version((struct wl_proxy *)manager), 0, NULL, toplevel); }
static inline void zxdg_toplevel_decoration_v1_set_mode(struct zxdg_toplevel_decoration_v1 *decoration, uint32_t mode)
{ wl_proxy_marshal_flags((struct wl_proxy *)decoration, ZXDG_TOPLEVEL_DECORATION_V1_SET_MODE, NULL, wl_proxy_get_version((struct wl_proxy *)decoration), 0, mode); }
static inline void zxdg_toplevel_decoration_v1_destroy(struct zxdg_toplevel_decoration_v1 *decoration)
{ wl_proxy_marshal_flags((struct wl_proxy *)decoration, ZXDG_TOPLEVEL_DECORATION_V1_DESTROY, NULL, wl_proxy_get_version((struct wl_proxy *)decoration), WL_MARSHAL_FLAG_DESTROY); }

#pragma pop_macro("global")

////////////////////////////////
//~ rjf: Backend Selection

typedef enum LNX_WM_Backend
{
  LNX_WM_Backend_X11,
  LNX_WM_Backend_Wayland,
}
LNX_WM_Backend;

global LNX_WM_Backend lnx_wm_backend = LNX_WM_Backend_X11;

////////////////////////////////
//~ rjf: Clipboard Tuning
//
// Both backends hand the clipboard payload to (or take it from) another process
// over a channel that process may never service - an X11 requestor that stops
// deleting the INCR property, a Wayland peer that never drains its end of the
// pipe. Every such wait is bounded so a misbehaving peer can only cost us a
// beat, never the whole UI.

#define LNX_WM_CLIPBOARD_TIMEOUT_US 1000000

////////////////////////////////
//~ rjf: X11 Types

#define MWM_HINTS_DECORATIONS (1L << 1)
#define MWM_DECOR_NONE 0

typedef struct LNX_MotifWMHints LNX_MotifWMHints;
struct LNX_MotifWMHints
{
  unsigned long flags;
  unsigned long functions;
  unsigned long decorations;
  long input_mode;
  unsigned long status;
};

#define LNX_WM_MOVERESIZE_SIZE_TOPLEFT     0
#define LNX_WM_MOVERESIZE_SIZE_TOP         1
#define LNX_WM_MOVERESIZE_SIZE_TOPRIGHT    2
#define LNX_WM_MOVERESIZE_SIZE_RIGHT       3
#define LNX_WM_MOVERESIZE_SIZE_BOTTOMRIGHT 4
#define LNX_WM_MOVERESIZE_SIZE_BOTTOM      5
#define LNX_WM_MOVERESIZE_SIZE_BOTTOMLEFT  6
#define LNX_WM_MOVERESIZE_SIZE_LEFT        7
#define LNX_WM_MOVERESIZE_MOVE             8
#define LNX_WM_MOVERESIZE_NONE             (-1)

#define LNX_WM_MAX_TITLE_BAR_CLIENT_AREAS 256

typedef struct LNX_WM_Window LNX_WM_Window;
struct LNX_WM_Window
{
  LNX_WM_Window *next;
  LNX_WM_Window *prev;
  Window window;
  XIC xic;
  XID counter_xid;
  U64 counter_value;
  B32 sync_request_pending;
  XID extended_counter_xid;
  S64 extended_counter_value;
  B32 extended_frame_pending;

  F32 custom_border_title_thickness;
  F32 custom_border_edge_thickness;
  U64 title_bar_client_area_count;
  Rng2F32 title_bar_client_areas[LNX_WM_MAX_TITLE_BAR_CLIENT_AREAS];
};

// rjf: an in-flight ICCCM INCR hand-off: a selection payload too large for a
// single X request, dripped to the requestor one property write at a time as it
// deletes the property to ask for more.
typedef struct LNX_WM_IncrSend LNX_WM_IncrSend;
struct LNX_WM_IncrSend
{
  LNX_WM_IncrSend *next;
  LNX_WM_IncrSend *prev;
  Arena *arena;
  Window requestor;
  Atom property;
  Atom type;
  String8 data;
  U64 off;
};

typedef struct LNX_WM_State LNX_WM_State;
struct LNX_WM_State
{
  Arena *arena;
  Display *display;
  XIM xim;
  LNX_WM_Window *first_window;
  LNX_WM_Window *last_window;
  LNX_WM_Window *free_window;
  Atom wm_delete_window_atom;
  Atom wm_sync_request_atom;
  Atom wm_sync_request_counter_atom;
  Atom motif_wm_hints_atom;
  Atom net_wm_moveresize_atom;
  Cursor cursors[WM_Cursor_COUNT];
  WM_Cursor last_set_cursor;
  WM_SystemInfo gfx_info;
  int wakeup_fd;
  Visual *window_visual;
  int window_depth;
  Colormap window_colormap;

  // rjf: clipboard
  Window clipboard_window;
  Atom clipboard_atom;
  Atom targets_atom;
  Atom timestamp_atom;
  Atom multiple_atom;
  Atom incr_atom;
  Atom utf8_string_atom;
  Atom text_atom;
  Atom text_plain_atom;
  Atom text_plain_utf8_atom;
  Atom clipboard_recv_atom;
  Arena *clipboard_arena;
  String8 clipboard_text;
  B32 clipboard_owned;
  Time last_event_time;
  U64 foreign_traffic_depth;
  int (*prev_error_handler)(Display *display, XErrorEvent *evt);
  LNX_WM_IncrSend *first_incr_send;
  LNX_WM_IncrSend *last_incr_send;
  LNX_WM_IncrSend *free_incr_send;
};

global LNX_WM_State *lnx_wm_state = 0;

internal LNX_WM_Window *lnx_window_from_x11window(Window window);
internal void lnx_window_finish_frame_sync(WM_Window handle);
internal int lnx_x11_error_handler(Display *display, XErrorEvent *evt);
internal void lnx_x11_service_selection_request(XSelectionRequestEvent *request);
internal void lnx_x11_advance_incr_sends(XPropertyEvent *evt);

////////////////////////////////
//~ rjf: Wayland Types

#define WL_WM_MAX_TITLE_BAR_CLIENT_AREAS 256

typedef struct WL_WM_Window WL_WM_Window;
struct WL_WM_Window
{
  WL_WM_Window *next;
  WL_WM_Window *prev;

  struct wl_surface *surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
  struct zxdg_toplevel_decoration_v1 *decoration;
  struct wl_egl_window *egl_window;

  Vec2S32 size;
  Vec2S32 pending_size;
  B32 fullscreen;
  B32 maximized;
  B32 activated;

  F32 custom_border_title_thickness;
  F32 custom_border_edge_thickness;
  U64 title_bar_client_area_count;
  Rng2F32 title_bar_client_areas[WL_WM_MAX_TITLE_BAR_CLIENT_AREAS];
};

// rjf: text flavors a wl_data_offer can advertise, ordered worst-to-best so the
// highest set bit picks the flavor we would rather read.
typedef enum WL_WM_MimeFlags
{
  WL_WM_MimeFlag_String       = (1<<0),
  WL_WM_MimeFlag_TextPlain    = (1<<1),
  WL_WM_MimeFlag_UTF8String   = (1<<2),
  WL_WM_MimeFlag_TextPlainUTF8= (1<<3),
}
WL_WM_MimeFlags;

typedef struct WL_WM_DataOffer WL_WM_DataOffer;
struct WL_WM_DataOffer
{
  WL_WM_DataOffer *next;
  WL_WM_DataOffer *prev;
  struct wl_data_offer *offer;
  WL_WM_MimeFlags mime_flags;
};

typedef struct WL_WM_State WL_WM_State;
struct WL_WM_State
{
  Arena *arena;

  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_compositor *compositor;
  struct wl_shm *shm;
  struct xdg_wm_base *wm_base;
  struct zxdg_decoration_manager_v1 *decoration_manager;
  struct wl_data_device_manager *data_device_manager;
  struct wl_data_device *data_device;
  struct wl_seat *seat;
  struct wl_pointer *pointer;
  struct wl_keyboard *keyboard;

  struct wl_cursor_theme *cursor_theme;
  struct wl_surface *cursor_surface;
  WM_Cursor last_set_cursor;
  WM_Cursor applied_cursor;
  U32 pointer_enter_serial;

  struct xkb_context *xkb_context;
  struct xkb_keymap *xkb_keymap;
  struct xkb_state *xkb_state;
  WM_Modifiers modifiers;

  int repeat_fd;
  S32 repeat_rate;
  S32 repeat_delay_ms;
  U32 repeat_key;

  WL_WM_Window *pointer_focus;
  WL_WM_Window *keyboard_focus;
  Vec2F32 pointer_pos;
  U32 last_input_serial;

  // rjf: clipboard
  struct wl_data_source *data_source;
  Arena *clipboard_arena;
  String8 clipboard_text;
  B32 clipboard_owned;
  WL_WM_DataOffer *first_offer;
  WL_WM_DataOffer *last_offer;
  WL_WM_DataOffer *free_offer;
  WL_WM_DataOffer *selection_offer;

  Arena *evt_arena;
  WM_EventList *evts;

  WL_WM_Window *first_window;
  WL_WM_Window *last_window;
  WL_WM_Window *free_window;

  WM_SystemInfo gfx_info;
  int wakeup_fd;
};

global WL_WM_State *wl_wm_state = 0;

internal WL_WM_Window *wl_window_from_surface(struct wl_surface *surface);

#endif // LINUX_WINDOW_MANAGER_H
