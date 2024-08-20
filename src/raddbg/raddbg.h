// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Frontend/UI Pass Tasks
//
// [x] fix HRESULTs
// [ ] fix escape char literals
//
// [ ] fix selecting hover eval, then hover eval disappearing, causing
//     busted focus, until a new hover eval is opened
// [ ] save view column pcts; generalize to being a first-class thing in
//     DF_View, e.g. by just having a string -> f32 store
// [ ] decay arrays to pointers in pointer/value comparison
// [ ] EVAL LOOKUP RULES -> currently going 0 -> rdis_count, but we need
// to prioritize the primary rdi
// [ ] EVAL SPACES - each rdi gets an rdi space, rdi space is passed to
// memory reads & so on, used to resolve to value space; REPLACES "mode"
//
// [ ] file overrides -> always pick most specific one! found with conflicting
//     overrides, e.g. C:/devel/ -> D:/devel/, but also C:/devel/foo ->
//     C:/devel/bar, etc.
//
// [ ] auto-scroll output window
// [ ] theme lister -> fonts & font sizes
// [ ] "Browse..." buttons should adopt a more relevant starting search path,
//     if possible
// [ ] move breakpoints to being a global thing, not nested to particular files
// [ ] visualize all breakpoints everywhere - source view should show up in
//     disasm, disasm should show up in source view, function should show up in
//     both, etc.
//  [ ] ** Function breakpoints should show up in the source listing. Without
//      them being visible, it is confusing when you run and you stop there,
//      because you're like "wait why did it stop" and then you later remember
//      that's because there was a function breakpoint there.
//
// [ ] n-row table selection, in watch window & other UIs, multi-selection
//     ctrl+C
//
// [ ] target/breakpoint/watch-pin reordering
//
// [ ] font lister
// [ ] per-panel font size overrides
//
// [ ] For the Scheduler window, it would be nice if you could dim or
//     folderize threads that are not your threads - eg., if a thread doesn't
//     have any resolved stack pointers in your executable code, then you can
//     ignore it when you are focusing on your own code. I don't know what the
//     best way to detect this is, other than by walking the call stack... one
//     way might be to just have a way to separate threads you've named from
//     threads you haven't? Or, there could even be a debugger-specific API
//     that you use to tag them. Just some way that would make it easier to
//     focus on your own threads.
//
// [ ] "concept key stack"; basically, any point in UI builder path has a stack
//     of active "concept keys", which can be used to e.g. build context menus
//     automatically (could just be a per-box attachment; right-click any
//     point, search up the tree and see the concept keys)
// [ ] ui_next_event(...), built-in focus filtering, no need to manually check
//     if(ui_is_focus_active())

////////////////////////////////
//~ rjf: Hot, Medium Priority Tasks (Low-Hanging-Fruit Features, UI Jank, Cleanup)
//
// [ ] Setting the code_font/main_font values to a font name doesn't work.
//     Should probably make note that you have to set it to a path to a TTF,
//     since that's not normally how Windows fonts work.
//
// [ ] "root" concept in hash store, which buckets keys & allows usage code to
//     jettison a collection of keys in retained mode fashion
//
// [ ] Jeff Notes
//  [ ] sort locals by appearance in source code (or maybe just debug info)
//  [ ] sum view rule
//  [ ] plot view rule
//  [ ] histogram view rule
//  [ ] max view rule
//  [ ] min view rule
//
// [ ] filesystem drag/drop support
// [ ] double-click vs. single-click for folder navigation, see if we can infer
// [ ] use backslashes on windows by default, forward slashes elsewhere
//
// [ ] investigate /DEBUG:FASTLINK - can we somehow alert that we do not
//     support it?
//
// [ ] ** Converter performance & heuristics for asynchronously doing it early
//
// [ ] visualize conversion failures
//
//  [ ] I was a little confused about what a profile file was. I understood
//      what the user file was, but the profile file sounded like it should
//      perhaps be per-project, yet it sounded like it was meant to be somewhat
//      global? I don't have any feedback here because it probably will make
//      sense once I use the debugger more, but I just thought I'd make a note
//      to say that I was confused about it after reading the manual, so
//      perhaps you could elaborate a little more on it in there.
//  [ ] It wasn't clear to me how you save a user or project file. I can see
//      how to load them, but not how you save them. Obviously I can just copy
//      the files myself in the shell, but it seemed weird that there was no
//      "save" option in the menus.
//
// [ ] Right-clicking on a thread in the Scheduler window pops up a context
//     menu, but you can't actually see it because the tooltip for the thread
//     draws on top of it, so you can't see the menu.
//
//  [ ] In a "hover watch" (where you hover over a variable and it shows a pop-
//      up watch window), if you expand an item near the bottom of the listing,
//      it will be clipped to the bottom of the listing instead of showing the
//      actual items (ie., it doesn't resize the listing based on what's
//      actually visible)
//
//  [ ] ** One very nice feature of RemedyBG that I use all the time is the
//      ability to put "$err, hr" into the watch window, which will just show
//      the value of GetLastError() as a string. This is super useful for
//      debugging, so you don't have to litter your own code with it.
//
//  [ ] Tooltip Coverage:
//   [ ] lock icon
//   [ ] "rotation arrow" icon next to executables
//
//  [ ] For theme editing, when you hove the mouse over a theme color entry and
//      it highlights that entry, it might help to temporarily change that
//      color to white (or the inverse of the background color, or whatever) so
//      that the user can see what things on the screen use that theme color.
//
//  [ ] Theme window should include font scaling. I was able to find the
//      command for increasing the font scale, but I imagine most people
//      wouldn't think to look there.
//  [ ] I had to go into the user file to change the font. That should probably
//      be in the theme window?
//
//  [ ] It'd be nice to have a "goto byte" option for source views, for jumping
//      to error messages that are byte-based instead of line-based.
//
// [ ] @feature debug info overrides (both path-based AND module-based)
//
// [ ] C++ virtual inheritance member visualization in watch window

////////////////////////////////
//~ rjf: Hot, Low Priority Tasks (UI Opinions, Less-Serious Jank, Preferences, Cleanup)
//
//  [ ] The hex format for color values in the config file was a real
//      mindbender. It's prefixed with "0x", so I was assuming it was either
//      Windows Big Endian (0xAARRGGBB) or Mac Little Endian (0xAABBGGRR). To
//      my surprise, it was neither - it was actually web format (RRGGBBAA),
//      which I was not expecting because that is normally written with a
//      number sign (#AARRGGBB) not an 0x.
//
//  [ ] Clicking on either side of a scroll bar is idiosyncratic. Normally,
//      that is "page up" / "page down", but here it is "smooth scroll upward"
//      / "smooth scroll downward" for some reason?
//
//  [ ]  can it ignore stepping into _RTC_CheckStackVars generated functions?
//  [ ]  mouse back button should make view to go back after I double clicked
//       on function to open it
//  [ ]  Alt+8 to switch to disassembly would be nice (regardless on which
//       panel was previous, don't want to use ctrl+, multiple times)
//       Alt+8 for disasm and Alt+6 for memory view are shortcuts I often use
//       in VS
//  [ ]  default font size is too small for me - not only source code, but
//       menus/tab/watch names (which don't resize). Maybe you could query
//       Windows for initial font size?
//  [ ]  icon fonts glyphs sometimes disappear for specific font size, but they
//       reappear if you go +1 higher or -1 lower. Mostly red triangle in watch
//       values for "unknown identifier". But also yellow arrow in call stack
//       disappears if font size gets too large.
//  [ ]  undo close tab would be nice. If not for everything, then at least
//       just for source files
// [ ] Jump table thunks, on code w/o /INCREMENTAL:NO

////////////////////////////////
//~ rjf: Hot, Feature Tasks (Not really "low priority" but less urgent than fixes)
//
// [ ] @eval_upgrade
//  [ ] new eval system; support strings, many address spaces, many debug
//      infos, wide/async transforms (e.g. diff(blob1, blob2))
//  [ ] collapse frontend visualization systems - source view, disasm view,
//      callstack, modules, scheduler, should *all* be flavors of watch view
//
// [ ] Fancy View Rules
//  [ ] table column boundaries should be checked against *AFTER* table
//      contents, not before
//  [ ] `array:(x, y)` - multidimensional array
//  [ ] `text[:lang]` - interpret memory as text, in lang `lang`
//  [ ] `disasm:arch` - interpret memory as machine code for isa `arch`
//  [ ] `memory` - view memory in usual memory hex-editor view
//  NOTE(rjf): When the visualization system is solid, layers like dasm, txti,
//  and so on can be dispensed with, as things like the source view, disasm
//  view, or memory view will simply be specializations of the general purpose
//  viz system.
//  [ ] view rule hook for standalone visualization ui, granted its own
//      tab
//
// [ ] search-in-all-files
//
// [ ] Memory View
//  [ ] memory view mutation controls
//  [ ] memory view user-made annotations
//
// [ ] undo/redo
// [ ] proper "go back" + "go forward" history navigations
//
// [ ] globally disable/configure default view rule-like things (string
//     viz for u8s in particular)
// [ ] globally disable/configure bp/ip lines in source view
//
// [ ] @feature processor/data breakpoints
// [ ] @feature automatically snap to search matches when searching source files
// [ ] automatically start search query with selected text
// [ ] @feature entity views: filtering & reordering

////////////////////////////////
//~ rjf: Cold, Clean-up Tasks That Probably Only Ryan Notices
// (E.G. Because They Are Code-Related Or Because Nobody Cares)
//
// [ ] @bug view-snapping in scroll-lists, accounting for mapping between
//     visual positions & logical positions (variably sized rows in watch,
//     table headers, etc.)
// [ ] @cleanup collapse DF_CfgNodes into just being MD trees, find another way
//     to encode config source - don't need it at every node
// [ ] @cleanup straighten out index/number space & types & terminology for
//     scroll lists
// [ ] @cleanup simplification pass over eval visualization pipeline & types,
//     including view rule hooks
// [ ] @cleanup naming pass over eval visualization part of the frontend,
//     "blocks" vs. "canvas" vs. "expansion" - etc.
// [ ] @cleanup central worker thread pool - eliminate per-layer thread pools
// [ ] @cleanup in the frontend, we are starting to have to pass down "DF_Window"
//     everywhere, because of per-window parameters (e.g. font rendering settings).
//     this is really better solved by implicit thread-local parameters, similar to
//     interaction registers, so that one window can "pick" all of the implicit
//     parameters, and then 99% of the UI code does not have to care.
// [ ] @cleanup eliminate explicit font parameters in the various ui paths (e.g.
//     code slice params)

////////////////////////////////
//~ rjf: Cold, Unsorted Notes (Deferred Until Existing Lists Mostly Exhausted)
//
// [ ] @feature types -> auto view rules (don't statefully fill view rules
//     given types, just query if no other view rule is present, & autofill
//     when editing)
// [ ] @feature eval system -> somehow evaluate breakpoint hit counts? "meta"
//     variables?
//
// [ ] @feature disasm view improvement features
//  [ ] visualize jump destinations in disasm
//
// [ ] @feature eval ui improvement features
//  [ ] serializing eval view maps
//  [ ] view rule editors in hover-eval
//  [ ] view rule hook coverage
//   [ ] `each:(expr addition)` - apply some additional expression to all
//        elements in an array/linked list would be useful to look at only a
//        subset of an array of complex structs
//   [ ] `slider:(min max)` view rule
//   [ ] `v2f32` view rule
//   [ ] `v3` view rule
//   [ ] `quat` view rule
//   [ ] `matrix` view rule
//   [ ] `audio` waveform view rule
//  [ ] smart scopes - expression operators for "grab me the first type X"
//  [ ] "pinning" watch expressions, to attach it to a particular ctrl_ctx
//
// [ ] @feature header file for target -> debugger communication; printf, log,
//     etc.
// [ ] @feature just-in-time debugging
// [ ] @feature step-out-of-loop
//
//-[ ] long-term future notes from martins
//  [ ] core dump saving/loading
//  [ ] parallel call stacks view
//  [ ] parallel watch view
//  [ ] mixed native/interpreted/jit debugging
//      - it seems python has a top-level linked list of interpreter states,
//        which should allow the debugger to map native callstacks to python
//        code
//
// [ ] fancy string runs can include "weakness" information for text truncation
//     ... can prioritize certain parts of strings to be truncated before
//     others. would be good for e.g. the middle of a path
// [ ] font cache eviction (both for font tags, closing fp handles, and
//     rasterizations)
// [ ] frontend speedup opportunities
//  [ ] tables in UI -> currently building per-row, could probably cut down on
//      # of boxes and # of draws by doing per-column in some cases?
//  [ ] font cache layer -> can probably cache (string*font*size) -> (run) too
//      (not just rasterization)... would save a *lot*, there is a ton of work
//      just in looking up & stitching stuff repeatedly
//  [ ] convert UI layout pass to not be naive recursive version
//  [ ] (big change) parallelize window ui build codepaths per-panel

////////////////////////////////
//~ rjf: Recently Completed Task Log
//
// [x] UI_NavActions, OS_Event -> UI_Event (single event stream)
// [x] better discoverability for view rules - have better help hover tooltip,
//     info on arguments, and better autocomplete lister
// [x] source view -> floating margin/line-nums
// [x] watch window reordering
// [x] standard way to filter
// [x] autocomplete lister should respect position in edited expression,
//     tabbing through should autocomplete but not exit, etc.
// [x] pipe failure-to-launch errors back to frontend
//  [x] bit more padding on the tabs
//  [x] unified top-level cursor/typing/lister helper
//  [x] collapse text cells & command lister & etc. into same codepath (?)
//  [x] page-up & page-down correct handling in keyboard nav
//  [x] interleaved src/dasm view
//  [x]  in watch window when I enter some new expression and then click mouse
//       away from cell, then it should behave the same as if I pressed enter.
//       Currently it does the same as if I have pressed esc and I have lost my
//       expression
//  [x]  pressing random keyboard keys in source code advances text cursor like
//       you were inputting text, very strange.
//  [x] It's confusing that ENTER is the way you expand and collapse things in
//      the watch window, but then also how you edit them if they are not
//      expandable? It seems like this should be consistent (one way to edit,
//      one way to expand/collapse, that are distinct)
//  [x] Dragging a window tab (like Locals or Registers or whatnot) and
//      canceling with ESC should revert the window tab to where it was.
//      Currently, it leaves the window tab reordered if you dragged over its
//      window and shuffled its position.
//  [x] ** I couldn't figure out how to really view threads in the debugger.
//      The only place I found a thread list was in "The Scheduler", but it
//      only lists threads by ID, which is hard to use. I can hover over them
//      to get the stack, which helps, but it would be much nicer if the top
//      function was displayed in the window by default next to the thread.
//  [x] ** It would be nice if thread listings displayed the name of the
//      thread, instead of just the ID.
// [x] TLS eval -> in-process-memory EXE info
// [x] unwinding -> in-process-memory EXE info
// [x] new fuzzy searching layer
// [x] robustify dbgi layer to renames (cache should not be based only on
//     path - must invalidate naturally when new filetime occurs)
// [x] rdi file regeneration too strict
// [x] raddbg jai.exe my_file.jai -- foobar -> raddbg consumes `--` incorrectly
// [x] mouse-driven way to complete file/folder selection, or more generally
// query completion
//  [x]  it would be nice to have "show in explorer" for right click on source
//       file tab (opens explorer & selects the file)
// [x] asan stepping breakage
//  [x]  what's up with decimal number coloring where every group of 3 are in
//       different color? can I turn it off? And why sometimes digits in number
//       start with brighter color, but sometimes with darker - shouldn't it
//       always have the same color ordering?
// [x] fix tabs-on-bottom positioning
// [x] colors: consistent tooltip styles (colors, font flags, etc.)
// [x] colors: scroll bars
// [x] colors: watch window navigation visuals
// [x] floating source view margin background/placement
// [x] "interaction root", or "group" ui_key, or something; used for menu bar interactions
// [x] theme colors -> more explicit about e.g. opaque backgrounds vs. floating
//     & scrollbars etc.
//  [x] Pressing the left mouse button on the menu bar and dragging does not
//      move through the menus as expected - instead, it opens the one you
//      clicked down on, then does nothing until you release, at which point it
//      opens the menu you released on.
//  [x] Similarly, pressing the left mouse button on a menu and dragging to an
//      item, then releasing, does not trigger that item as expected. Instead,
//      it is a nop, and it waits for you to click again on the item.
//  [x] Using the word "symbol" in "Code (Symbol)" seems like a bad idea, since
//      you're referring to non-identifier characters, but in a debugger
//      "symbol" usually means something defined in the debug information.
//  [x] I couldn't figure out how to affect the "dim" color in constants that
//      have alternating bright/dim letters to show sections of a number. Is
//      this in the theme colors somewhere?
//
//  [x] ** Scrollbars are barely visible for me, for some reason. I could not
//      find anything in the theme that would fill them with a solid, bright
//      color. Instead they are just a thin outline and the same color as the
//      scroll bar background.
//
//  [x] Many of the UI elements, like the menus, would like better if they had
//      a little bit of margin. Having the text right next to the edges, and
//      with no line spacing, makes it harder to read things quickly.
// [x] colors: memory view
//  [x] Hitting ESC during a color picker drag should abort the color picking
//      and revert to the previous color. Currently, it just accepts the last
//      drag result as the new color.
//  [x] It was not clear to me why a small "tab picker" appeared when I got to
//      a certain number of tabs. It seemed to appear even if the tabs were
//      quite large, and there was no need to a drop-down menu to pick them. It
//      feels like either it should always be there, or it should only show up
//      if at least one tab gets small enough to have its name cut off?
//  [x] I found the "context menu" convention to be confusing. For example, if
//      I left-click on a tab, it selects the tab. If I right-click on a tab,
//      it opens the context menu. However, if I left-click on a module, it
//      opens the context window. It seems like maybe menus should be right,
//      and left should do the default action, more consistently?
//
//  [x] double click on procedure in procedures tab to jump to source
//  [x] highlighted text & ctrl+f -> auto-fill search query
//  [x] double-click any part of frame in callstack view -> snap to function
//  [x] Menus take too long to show up. I would prefer it if they were instant.
//      The animation doesn't really provide any useful cues, since I know
//      where the menu came from.
// [x] user settings (ui & functionality - generally need a story for it)
//  [x] hover animations
//  [x] press animations
//  [x] focus animations
//  [x] tooltip animations
//  [x] context menu animations
//  [x] scrolling animations
//  [x] background blur
//  [x] tab width
//  [x] ** In the call stack, I would like to be able to click quickly and move
//      around the stack. Right now, you can do that with the first and third
//      column, but the second column drops down a context menu. Since right
//      click is already for context menus, can it not just be that double-
//      clicking any column jumps to that stack frame?
//
//  [x] ** I find it really hard to read the code with the heavyweight lines
//      running through it for breakpoints and stepping and things. Is there a
//      way to turn the lines off? AFAICT they are based on thread and
//      breakpoint color, so you can't really control the line drawing? I might
//      be fine with them, but they would have to be much more light (like
//      alpha 0.1 or something)
//  [x]  zooming behaves very strangely - sometimes it zooms source code,
//       sometimes both source code and menu/tab/watch font size, sometimes
//       just menu/tab/watch font size not source size.
// [x] colors: fill out rest of theme presets for new theme setup
//  [x] I LOVE ALT-W to add watch under cursor, but I would prefer to have it
//      add what's under the MOUSE cursor instead of the keyboard cursor. Can
//      we get a command for that so I can bind ALT-W to that instead?
// [x] editing multiple bindings for commands
// [x] inline breakpoint hit_count
//  [x] to count hit counts, resolve all bps to addresses, check addresses
//      against stopper thread's
//
// [x] PDB files distributed with the build are not found by DbgHelp!!!
// [x] Jai compiler debugging crash

#ifndef RADDBG_H
#define RADDBG_H

////////////////////////////////
//~ rjf: Top-Level Execution Types

typedef enum ExecMode
{
  ExecMode_Normal,
  ExecMode_IPCSender,
  ExecMode_Converter,
  ExecMode_Help,
}
ExecMode;

typedef struct IPCInfo IPCInfo;
struct IPCInfo
{
  U64 msg_size;
};

////////////////////////////////
//~ rjf: Globals

//- rjf: IPC resources
#define IPC_SHARED_MEMORY_BUFFER_SIZE MB(4)
StaticAssert(IPC_SHARED_MEMORY_BUFFER_SIZE > sizeof(IPCInfo), ipc_buffer_size_requirement);
global OS_Handle ipc_signal_semaphore = {0};
global OS_Handle ipc_lock_semaphore = {0};
global U8 *ipc_shared_memory_base = 0;
global U8  ipc_s2m_ring_buffer[MB(4)] = {0};
global U64 ipc_s2m_ring_write_pos = 0;
global U64 ipc_s2m_ring_read_pos = 0;
global OS_Handle ipc_s2m_ring_mutex = {0};
global OS_Handle ipc_s2m_ring_cv = {0};

//- rjf: last focused window
global DF_Handle last_focused_window = {0};

//- rjf: frame time history
global U64 frame_time_us_history[64] = {0};
global U64 frame_time_us_history_idx = 0;

//- rjf: main thread log
global Log *main_thread_log = 0;
global String8 main_thread_log_path = {0};

////////////////////////////////
//~ rjf: Frontend Entry Points

internal void update_and_render(OS_Handle repaint_window_handle, void *user_data);

#endif // RADDBG_H
