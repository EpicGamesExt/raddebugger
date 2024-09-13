// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Frontend/UI Pass Tasks
//
// [ ] empty user file causing failure to launch
//
// [ ] frontend entities vs. engine entities
//  - currently, the engine has entities like "watch", and the frontend
//    has entities like "windows", "panels", and "views".
//  - because "watch" entities ideally have a hierarchical relationship
//    with windows, panels, and views (enabling things like drag/drop
//    from watch window -> tab, or tab -> watch window, trivially), it
//    would be much better if all entities could collapse into engine
//    entities.
//  - now, the frontend requires various specialized resources for things
//    like windows, so what I am thinking is that the engine just controls
//    all of the stateful windows/panel/view/watch mechanisms, and then
//    the frontend pure-functionally queries stuff like os/r handles
//    on-demand, and then prunes them, immediate-mode cache style.
//
// [ ] transient view timeout releasing
//
// [ ] save view column pcts; generalize to being a first-class thing in
//     RD_View, e.g. by just having a string -> f32 store
// [ ] decay arrays to pointers in pointer/value comparison
// [ ] EVAL LOOKUP RULES -> currently going 0 -> rdis_count, but we need
// to prioritize the primary rdi
//
// [ ] file overrides -> always pick most specific one! found with conflicting
//     overrides, e.g. C:/devel/ -> D:/devel/, but also C:/devel/foo ->
//     C:/devel/bar, etc.
//
// [ ] auto-scroll output window
// [ ] theme lister -> fonts & font sizes
// [ ] "Browse..." buttons should adopt a more relevant starting search path,
//     if possible
// [ ] visualize all breakpoints everywhere - source view should show up in
//     disasm, disasm should show up in source view, function should show up in
//     both, etc.
//  [ ] ** Function breakpoints should show up in the source listing. Without
//      them being visible, it is confusing when you run and you stop there,
//      because you're like "wait why did it stop" and then you later remember
//      that's because there was a function breakpoint there.
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
//
// [ ] Fancy View Rules
//  [ ] table column boundaries should be checked against *AFTER* table
//      contents, not before
//  [ ] `array:(x, y)` - multidimensional array
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
// [ ] @cleanup straighten out index/number space & types & terminology for
//     scroll lists
// [ ] @cleanup central worker thread pool - eliminate per-layer thread pools
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
//
//- 2024/8/29
//
// [x] fix HRESULTs
// [x] fix escape char literals
// [x] eval: indexing into string literals
// [x] fix incorrectly consuming keyboard inputs, preventing fallback-to-filtering, when
//     selecting null selection in watch views
// [x] ui_next_event(...), built-in focus filtering, no need to manually check
//     if(ui_is_focus_active())
//  [x] Theme window should include font scaling. I was able to find the
//      command for increasing the font scale, but I imagine most people
//      wouldn't think to look there.
// [x] n-row table selection, in watch window & other UIs, multi-selection
//     ctrl+C
// [x] target/breakpoint/watch-pin reordering
// [x] move breakpoints to being a global thing, not nested to particular files
// [x] EVAL SPACES - each rdi gets an rdi space, rdi space is passed to
// memory reads & so on, used to resolve to value space; REPLACES "mode"
// [x] fix selecting hover eval, then hover eval disappearing, causing
//     busted focus, until a new hover eval is opened
//  [x] `text[:lang]` - interpret memory as text, in lang `lang`
//  [x] `disasm:arch` - interpret memory as machine code for isa `arch`
//  [x] `memory` - view memory in usual memory hex-editor view
//  NOTE(rjf): When the visualization system is solid, layers like dasm, txti,
//  and so on can be dispensed with, as things like the source view, disasm
//  view, or memory view will simply be specializations of the general purpose
//  viz system.
//  [x] view rule hook for standalone visualization ui, granted its own
//      tab
//  [x] collapse frontend visualization systems - source view, disasm view,
//      callstack, modules, scheduler, should *all* be flavors of watch view
// [x] globally disable/configure bp/ip lines in source view
// [x] @cleanup naming pass over eval visualization part of the frontend,
//     "blocks" vs. "canvas" vs. "expansion" - etc.
// [x] @cleanup collapse DF_CfgNodes into just being MD trees, find another way
//     to encode config source - don't need it at every node
// [x] @cleanup in the frontend, we are starting to have to pass down "RD_Window"
//     everywhere, because of per-window parameters (e.g. font rendering settings).
//     this is really better solved by implicit thread-local parameters, similar to
//     interaction registers, so that one window can "pick" all of the implicit
//     parameters, and then 99% of the UI code does not have to care.
// [x] @cleanup simplification pass over eval visualization pipeline & types,
//     including view rule hooks
// [x] engine/frontend commands situation
//  - currently, there is an interesting bifurcation of commands in the
//    frontend; you can either push a command *at a root level*, or push a
//    command to a locally-accessible list if you want that command to run
//    on the same frame (root level commands are deferred by a frame, since
//    the engine must see them first).
//  - things would be simpler if there was only a single "push command"
//    mechanism, and codepaths only ever saw these commands at most once.
//    this would require an alternate strategy of the initial "gather" of
//    commands, and instead it would just be a global queue, or something...
//    it is a little weird since commands are not just consumed in order...
//  - this will clean up the various different ways that codepaths
//    parameterize commands.
// [x] command params -> d_regs
//  - currently there are two almost-identical concepts relating to commands:
//    the parameters struct, and D_Regs. D_Regs is a registers struct which
//    is used to manage a stack of contextual information in various debugger
//    codepaths. it is used so that codepaths can register information they
//    know about, without passing it down to everyone explicitly - but those
//    later codepaths can still pass that information along. e.g. a window
//    calls into a watch window, watch window calls into visualizer, visualizer
//    pushes command, which needs to pass which window it occurred on along.
//  - i think D_Regs needs to expand a bit in order to encompass all of the
//    things that the command parameters were being used for, but at that point
//    commands can just be a spec * regs, and then the push-command API can
//    just have ways of overriding regs values explicitly, when the codepath
//    needs to be opinionated about which things are affected by which commands

////////////////////////////////
//~ rjf: Build Options

#define BUILD_VERSION_MAJOR 0
#define BUILD_VERSION_MINOR 9
#define BUILD_VERSION_PATCH 12
#define BUILD_RELEASE_PHASE_STRING_LITERAL "ALPHA"
#define BUILD_TITLE "The RAD Debugger"
#define OS_FEATURE_GRAPHICAL 1

#define R_INIT_MANUAL 1
#define TEX_INIT_MANUAL 1
#define GEO_INIT_MANUAL 1
#define FNT_INIT_MANUAL 1
#define D_INIT_MANUAL 1
#define RD_INIT_MANUAL 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [lib]
#include "third_party/rad_lzb_simple/rad_lzb_simple.h"
#include "third_party/rad_lzb_simple/rad_lzb_simple.c"
#include "lib_raddbg_markup/raddbg_markup.h"
#include "lib_raddbg_markup/raddbg_markup.c"

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "task_system/task_system.h"
#include "ico/ico.h"
#include "rdi_format/rdi_format_local.h"
#include "rdi_make/rdi_make_local.h"
#include "mdesk/mdesk.h"
#include "hash_store/hash_store.h"
#include "file_stream/file_stream.h"
#include "text_cache/text_cache.h"
#include "mutable_text/mutable_text.h"
#include "path/path.h"
#include "coff/coff.h"
#include "pe/pe.h"
#include "codeview/codeview.h"
#include "codeview/codeview_stringize.h"
#include "msf/msf.h"
#include "pdb/pdb.h"
#include "pdb/pdb_stringize.h"
#include "rdi_from_pdb/rdi_from_pdb.h"
#include "regs/regs.h"
#include "regs/rdi/regs_rdi.h"
#include "dbgi/dbgi.h"
#include "dasm_cache/dasm_cache.h"
#include "fuzzy_search/fuzzy_search.h"
#include "demon/demon_inc.h"
#include "eval/eval_inc.h"
#include "eval_visualization/eval_visualization_inc.h"
#include "ctrl/ctrl_inc.h"
#include "font_provider/font_provider_inc.h"
#include "render/render_inc.h"
#include "texture_cache/texture_cache.h"
#include "geo_cache/geo_cache.h"
#include "font_cache/font_cache.h"
#include "draw/draw.h"
#include "ui/ui_inc.h"
#include "dbg_engine/dbg_engine_inc.h"
#include "raddbg/raddbg_inc.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "task_system/task_system.c"
#include "ico/ico.c"
#include "rdi_format/rdi_format_local.c"
#include "rdi_make/rdi_make_local.c"
#include "mdesk/mdesk.c"
#include "hash_store/hash_store.c"
#include "file_stream/file_stream.c"
#include "text_cache/text_cache.c"
#include "mutable_text/mutable_text.c"
#include "path/path.c"
#include "coff/coff.c"
#include "pe/pe.c"
#include "codeview/codeview.c"
#include "codeview/codeview_stringize.c"
#include "msf/msf.c"
#include "pdb/pdb.c"
#include "pdb/pdb_stringize.c"
#include "rdi_from_pdb/rdi_from_pdb.c"
#include "regs/regs.c"
#include "regs/rdi/regs_rdi.c"
#include "dbgi/dbgi.c"
#include "dasm_cache/dasm_cache.c"
#include "fuzzy_search/fuzzy_search.c"
#include "demon/demon_inc.c"
#include "eval/eval_inc.c"
#include "eval_visualization/eval_visualization_inc.c"
#include "ctrl/ctrl_inc.c"
#include "font_provider/font_provider_inc.c"
#include "render/render_inc.c"
#include "texture_cache/texture_cache.c"
#include "geo_cache/geo_cache.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"
#include "ui/ui_inc.c"
#include "dbg_engine/dbg_engine_inc.c"
#include "raddbg/raddbg_inc.c"

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

////////////////////////////////
//~ rjf: IPC Signaler Thread

internal void
ipc_signaler_thread__entry_point(void *p)
{
  for(;;)
  {
    if(os_semaphore_take(ipc_signal_semaphore, max_U64))
    {
      if(os_semaphore_take(ipc_lock_semaphore, max_U64))
      {
        IPCInfo *ipc_info = (IPCInfo *)ipc_shared_memory_base;
        String8 msg = str8((U8 *)(ipc_info+1), ipc_info->msg_size);
        msg.size = Min(msg.size, IPC_SHARED_MEMORY_BUFFER_SIZE - sizeof(IPCInfo));
        OS_MutexScope(ipc_s2m_ring_mutex) for(;;)
        {
          U64 unconsumed_size = ipc_s2m_ring_write_pos - ipc_s2m_ring_read_pos;
          U64 available_size = (sizeof(ipc_s2m_ring_buffer) - unconsumed_size);
          if(available_size >= sizeof(U64)+sizeof(msg.size))
          {
            ipc_s2m_ring_write_pos += ring_write_struct(ipc_s2m_ring_buffer, sizeof(ipc_s2m_ring_buffer), ipc_s2m_ring_write_pos, &msg.size);
            ipc_s2m_ring_write_pos += ring_write(ipc_s2m_ring_buffer, sizeof(ipc_s2m_ring_buffer), ipc_s2m_ring_write_pos, msg.str, msg.size);
            break;
          }
          os_condition_variable_wait(ipc_s2m_ring_cv, ipc_s2m_ring_mutex, max_U64);
        }
        os_condition_variable_broadcast(ipc_s2m_ring_cv);
        os_send_wakeup_event();
        ipc_info->msg_size = 0;
        os_semaphore_drop(ipc_lock_semaphore);
      }
    }
  }
}

////////////////////////////////
//~ rjf: Ctrl -> Main Thread Wakeup Hook

internal CTRL_WAKEUP_FUNCTION_DEF(wakeup_hook_ctrl)
{
  os_send_wakeup_event();
}

////////////////////////////////
//~ rjf: Per-Frame Entry Point

internal B32
frame(void)
{
  rd_frame();
  return rd_state->quit;
}

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmd_line)
{
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: windows -> turn off output handles, as we need to control those for target processes
#if OS_WINDOWS
  HANDLE output_handles[3] =
  {
    GetStdHandle(STD_INPUT_HANDLE),
    GetStdHandle(STD_OUTPUT_HANDLE),
    GetStdHandle(STD_ERROR_HANDLE),
  };
  for(U64 idx = 0; idx < ArrayCount(output_handles); idx += 1)
  {
    B32 duplicate = 0;
    for(U64 idx2 = 0; idx2 < idx; idx2 += 1)
    {
      if(output_handles[idx2] == output_handles[idx])
      {
        duplicate = 1;
        break;
      }
    }
    if(duplicate)
    {
      output_handles[idx] = 0;
    }
  }
  for(U64 idx = 0; idx < ArrayCount(output_handles); idx += 1)
  {
    if(output_handles[idx] != 0)
    {
      CloseHandle(output_handles[idx]);
    }
  }
  SetStdHandle(STD_INPUT_HANDLE, 0);
  SetStdHandle(STD_OUTPUT_HANDLE, 0);
  SetStdHandle(STD_ERROR_HANDLE, 0);
#endif
  
  //- rjf: unpack command line arguments
  ExecMode exec_mode = ExecMode_Normal;
  B32 auto_run = 0;
  B32 auto_step = 0;
  B32 jit_attach = 0;
  U64 jit_pid = 0;
  U64 jit_code = 0;
  U64 jit_addr = 0;
  {
    if(cmd_line_has_flag(cmd_line, str8_lit("ipc")))
    {
      exec_mode = ExecMode_IPCSender;
    }
    else if(cmd_line_has_flag(cmd_line, str8_lit("convert")))
    {
      exec_mode = ExecMode_Converter;
    }
    else if(cmd_line_has_flag(cmd_line, str8_lit("?")) ||
            cmd_line_has_flag(cmd_line, str8_lit("help")))
    {
      exec_mode = ExecMode_Help;
    }
    auto_run = cmd_line_has_flag(cmd_line, str8_lit("auto_run"));
    auto_step = cmd_line_has_flag(cmd_line, str8_lit("auto_step"));
    String8 jit_pid_string = cmd_line_string(cmd_line, str8_lit("jit_pid"));
    String8 jit_code_string = cmd_line_string(cmd_line, str8_lit("jit_code"));
    String8 jit_addr_string = cmd_line_string(cmd_line, str8_lit("jit_addr"));
    try_u64_from_str8_c_rules(jit_pid_string, &jit_pid);
    try_u64_from_str8_c_rules(jit_code_string, &jit_code);
    try_u64_from_str8_c_rules(jit_addr_string, &jit_addr);
    jit_attach = (jit_addr != 0);
  }
  
  //- rjf: set up layers
  ctrl_set_wakeup_hook(wakeup_hook_ctrl);
  
  //- rjf: dispatch to top-level codepath based on execution mode
  switch(exec_mode)
  {
    //- rjf: normal execution
    default:
    case ExecMode_Normal:
    {
      //- rjf: manual layer initialization
      {
        r_init(cmd_line);
        tex_init();
        geo_init();
        fnt_init();
        d_init();
        rd_init(cmd_line);
      }
      
      //- rjf: setup initial target from command line args
      {
        String8List args = cmd_line->inputs;
        if(args.node_count > 0 && args.first->string.size != 0)
        {
          Temp scratch = scratch_begin(0, 0);
          RD_Entity *target = rd_entity_alloc(rd_entity_root(), RD_EntityKind_Target);
          rd_entity_equip_cfg_src(target, RD_CfgSrc_CommandLine);
          String8List passthrough_args_list = {0};
          for(String8Node *n = args.first->next; n != 0; n = n->next)
          {
            str8_list_push(scratch.arena, &passthrough_args_list, n->string);
          }
          
          // rjf: get current path
          String8 current_path = os_get_current_path(scratch.arena);
          
          // rjf: equip exe
          if(args.first->string.size != 0)
          {
            String8 exe_name = args.first->string;
            RD_Entity *exe = rd_entity_alloc(target, RD_EntityKind_Executable);
            PathStyle style = path_style_from_str8(exe_name);
            if(style == PathStyle_Relative)
            {
              exe_name = push_str8f(scratch.arena, "%S/%S", current_path, exe_name);
              exe_name = path_normalized_from_string(scratch.arena, exe_name);
            }
            rd_entity_equip_name(exe, exe_name);
          }
          
          // rjf: equip working directory
          String8 path_part_of_arg = str8_chop_last_slash(args.first->string);
          if(path_part_of_arg.size != 0)
          {
            String8 path = push_str8f(scratch.arena, "%S/", path_part_of_arg);
            RD_Entity *wdir = rd_entity_alloc(target, RD_EntityKind_WorkingDirectory);
            rd_entity_equip_name(wdir, path);
          }
          
          // rjf: equip args
          StringJoin join = {str8_lit(""), str8_lit(" "), str8_lit("")};
          String8 args_str = str8_list_join(scratch.arena, &passthrough_args_list, &join);
          if(args_str.size != 0)
          {
            RD_Entity *args_entity = rd_entity_alloc(target, RD_EntityKind_Arguments);
            rd_entity_equip_name(args_entity, args_str);
          }
          scratch_end(scratch);
        }
      }
      
      //- rjf: set up shared resources for ipc to this instance; launch IPC signaler thread
      {
        Temp scratch = scratch_begin(0, 0);
        U32 instance_pid = os_get_process_info()->pid;
        String8 ipc_shared_memory_name = push_str8f(scratch.arena, "_raddbg_ipc_shared_memory_%i_", instance_pid);
        String8 ipc_signal_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_signal_semaphore_%i_", instance_pid);
        String8 ipc_lock_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_lock_semaphore_%i_", instance_pid);
        OS_Handle ipc_shared_memory = os_shared_memory_alloc(IPC_SHARED_MEMORY_BUFFER_SIZE, ipc_shared_memory_name);
        ipc_shared_memory_base = (U8 *)os_shared_memory_view_open(ipc_shared_memory, r1u64(0, IPC_SHARED_MEMORY_BUFFER_SIZE));
        ipc_signal_semaphore = os_semaphore_alloc(0, 1, ipc_signal_semaphore_name);
        ipc_lock_semaphore = os_semaphore_alloc(1, 1, ipc_lock_semaphore_name);
        ipc_s2m_ring_mutex = os_mutex_alloc();
        ipc_s2m_ring_cv = os_condition_variable_alloc();
        IPCInfo *ipc_info = (IPCInfo *)ipc_shared_memory_base;
        MemoryZeroStruct(ipc_info);
        os_thread_launch(ipc_signaler_thread__entry_point, 0, 0);
        scratch_end(scratch);
      }
      
      //- rjf: main application loop
      {
        for(B32 quit = 0; !quit;)
        {
          //- rjf: consume IPC messages, dispatch UI commands
          {
            Temp scratch = scratch_begin(0, 0);
            B32 consumed = 0;
            String8 msg = {0};
            OS_MutexScope(ipc_s2m_ring_mutex)
            {
              U64 unconsumed_size = ipc_s2m_ring_write_pos - ipc_s2m_ring_read_pos;
              if(unconsumed_size >= sizeof(U64))
              {
                consumed = 1;
                ipc_s2m_ring_read_pos += ring_read_struct(ipc_s2m_ring_buffer, sizeof(ipc_s2m_ring_buffer), ipc_s2m_ring_read_pos, &msg.size);
                msg.size = Min(msg.size, unconsumed_size);
                msg.str = push_array(scratch.arena, U8, msg.size);
                ipc_s2m_ring_read_pos += ring_read(ipc_s2m_ring_buffer, sizeof(ipc_s2m_ring_buffer), ipc_s2m_ring_read_pos, msg.str, msg.size);
              }
            }
            if(consumed)
            {
              os_condition_variable_broadcast(ipc_s2m_ring_cv);
            }
            if(msg.size != 0)
            {
              log_infof("ipc_msg: \"%S\"", msg);
              RD_Window *dst_window = rd_state->first_window;
              for(RD_Window *window = dst_window; window != 0; window = window->next)
              {
                if(os_window_is_focused(window->os))
                {
                  dst_window = window;
                  break;
                }
              }
              if(dst_window != 0)
              {
                dst_window->window_temporarily_focused_ipc = 1;
                U64 first_space_pos = str8_find_needle(msg, 0, str8_lit(" "), 0);
                String8 cmd_kind_name_string = str8_prefix(msg, first_space_pos);
                String8 cmd_args_string = str8_skip_chop_whitespace(str8_skip(msg, first_space_pos));
                RD_CmdKindInfo *cmd_kind_info = rd_cmd_kind_info_from_string(cmd_kind_name_string);
                if(cmd_kind_info != &rd_nil_cmd_kind_info) RD_RegsScope()
                {
                  if(dst_window != rd_window_from_handle(rd_regs()->window))
                  {
                    rd_regs()->window = rd_handle_from_window(dst_window);
                    rd_regs()->panel  = rd_handle_from_panel(dst_window->focused_panel);
                    rd_regs()->view   = dst_window->focused_panel->selected_tab_view;
                  }
                  rd_regs_fill_slot_from_string(cmd_kind_info->query.slot, cmd_args_string);
                  rd_push_cmd(cmd_kind_name_string, rd_regs());
                  rd_request_frame();
                }
                else
                {
                  log_user_errorf("\"%S\" is not a command.", cmd_kind_name_string);
                  rd_request_frame();
                }
              }
            }
            scratch_end(scratch);
          }
          
          //- rjf: update
          quit = update();
          
          //- rjf: auto run
          if(auto_run)
          {
            auto_run = 0;
            rd_cmd(RD_CmdKind_LaunchAndRun);
          }
          
          //- rjf: auto step
          if(auto_step)
          {
            auto_step = 0;
            rd_cmd(RD_CmdKind_StepInto);
          }
          
          //- rjf: jit attach
          if(jit_attach)
          {
            jit_attach = 0;
            rd_cmd(RD_CmdKind_Attach, .pid = jit_pid);
          }
        }
      }
      
    }break;
    
    //- rjf: inter-process communication message sender
    case ExecMode_IPCSender:
    {
      Temp scratch = scratch_begin(0, 0);
      
      //- rjf: grab explicit PID argument
      U32 dst_pid = 0;
      if(cmd_line_has_argument(cmd_line, str8_lit("pid")))
      {
        String8 dst_pid_string = cmd_line_string(cmd_line, str8_lit("pid"));
        U64 dst_pid_u64 = 0;
        if(dst_pid_string.size != 0 &&
           try_u64_from_str8_c_rules(dst_pid_string, &dst_pid_u64))
        {
          dst_pid = (U32)dst_pid_u64;
        }
      }
      
      //- rjf: no explicit PID? -> find PID to send message to, by looking for other raddbg instances
      if(dst_pid == 0)
      {
        U32 this_pid = os_get_process_info()->pid;
        DMN_ProcessIter it = {0};
        dmn_process_iter_begin(&it);
        for(DMN_ProcessInfo info = {0}; dmn_process_iter_next(scratch.arena, &it, &info);)
        {
          if(str8_match(str8_skip_last_slash(str8_chop_last_dot(cmd_line->exe_name)), str8_skip_last_slash(str8_chop_last_dot(info.name)), StringMatchFlag_CaseInsensitive) &&
             this_pid != info.pid)
          {
            dst_pid = info.pid;
            break;
          }
        }
        dmn_process_iter_end(&it);
      }
      
      //- rjf: grab destination instance's shared memory resources
      String8 ipc_shared_memory_name = push_str8f(scratch.arena, "_raddbg_ipc_shared_memory_%i_", dst_pid);
      String8 ipc_signal_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_signal_semaphore_%i_", dst_pid);
      String8 ipc_lock_semaphore_name = push_str8f(scratch.arena, "_raddbg_ipc_lock_semaphore_%i_", dst_pid);
      OS_Handle ipc_shared_memory = os_shared_memory_alloc(IPC_SHARED_MEMORY_BUFFER_SIZE, ipc_shared_memory_name);
      ipc_shared_memory_base = (U8 *)os_shared_memory_view_open(ipc_shared_memory, r1u64(0, IPC_SHARED_MEMORY_BUFFER_SIZE));
      ipc_signal_semaphore = os_semaphore_alloc(0, 1, ipc_signal_semaphore_name);
      ipc_lock_semaphore = os_semaphore_alloc(1, 1, ipc_lock_semaphore_name);
      
      //- rjf: got resources -> write message
      if(ipc_shared_memory_base != 0 &&
         os_semaphore_take(ipc_lock_semaphore, max_U64))
      {
        IPCInfo *ipc_info = (IPCInfo *)ipc_shared_memory_base;
        U8 *buffer = (U8 *)(ipc_info+1);
        U64 buffer_max = IPC_SHARED_MEMORY_BUFFER_SIZE - sizeof(IPCInfo);
        StringJoin join = {str8_lit(""), str8_lit(" "), str8_lit("")};
        String8 msg = str8_list_join(scratch.arena, &cmd_line->inputs, &join);
        ipc_info->msg_size = Min(buffer_max, msg.size);
        MemoryCopy(buffer, msg.str, ipc_info->msg_size);
        os_semaphore_drop(ipc_signal_semaphore);
        os_semaphore_drop(ipc_lock_semaphore);
      }
      
      scratch_end(scratch);
    }break;
    
    //- rjf: built-in pdb/dwarf -> rdi converter mode
    case ExecMode_Converter:
    {
      Temp scratch = scratch_begin(0, 0);
      
      //- rjf: parse arguments
      P2R_User2Convert *user2convert = p2r_user2convert_from_cmdln(scratch.arena, cmd_line);
      
      //- rjf: open output file
      String8 output_name = push_str8_copy(scratch.arena, user2convert->output_name);
      OS_Handle out_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_Write, output_name);
      B32 out_file_is_good = !os_handle_match(out_file, os_handle_zero());
      
      //- rjf: convert
      P2R_Convert2Bake *convert2bake = 0;
      if(out_file_is_good)
      {
        convert2bake = p2r_convert(scratch.arena, user2convert);
      }
      
      //- rjf: bake
      P2R_Bake2Serialize *bake2srlz = 0;
      ProfScope("bake")
      {
        bake2srlz = p2r_bake(scratch.arena, convert2bake);
      }
      
      //- rjf: serialize
      P2R_Serialize2File *srlz2file = 0;
      ProfScope("serialize")
      {
        srlz2file = push_array(scratch.arena, P2R_Serialize2File, 1);
        srlz2file->bundle = rdim_serialized_section_bundle_from_bake_results(&bake2srlz->bake_results);
      }
      
      //- rjf: compress
      P2R_Serialize2File *srlz2file_compressed = srlz2file;
      if(cmd_line_has_flag(cmd_line, str8_lit("compress"))) ProfScope("compress")
      {
        srlz2file_compressed = push_array(scratch.arena, P2R_Serialize2File, 1);
        srlz2file_compressed = p2r_compress(scratch.arena, srlz2file);
      }
      
      //- rjf: serialize
      String8List blobs = rdim_file_blobs_from_section_bundle(scratch.arena, &srlz2file_compressed->bundle);
      
      //- rjf: write
      if(out_file_is_good)
      {
        U64 off = 0;
        for(String8Node *n = blobs.first; n != 0; n = n->next)
        {
          os_file_write(out_file, r1u64(off, off+n->string.size), n->string.str);
          off += n->string.size;
        }
      }
      
      //- rjf: close output file
      os_file_close(out_file);
      
      scratch_end(scratch);
    }break;
    
    //- rjf: help message box
    case ExecMode_Help:
    {
      os_graphical_message(0,
                           str8_lit("The RAD Debugger - Help"),
                           str8_lit("The following options may be used when starting the RAD Debugger from the command line:\n\n"
                                    "--user:<path>\n"
                                    "Use to specify the location of a user file which should be used. User files are used to store settings for users, including window and panel setups, path mapping, and visual settings. If this file does not exist, it will be created as necessary. This file will be autosaved as user-related changes are made.\n\n"
                                    "--project:<path>\n"
                                    "Use to specify the location of a project file which should be used. Project files are used to store settings for users and projects. If this file does not exist, it will be created as necessary. This file will be autosaved as project-related changes are made.\n\n"
                                    "--auto_step\n"
                                    "This will step into all targets after the debugger initially starts.\n\n"
                                    "--auto_run\n"
                                    "This will run all targets after the debugger initially starts.\n\n"
                                    "--ipc <command>\n"
                                    "This will launch the debugger in the non-graphical IPC mode, which is used to communicate with another running instance of the debugger. The debugger instance will launch, send the specified command, then immediately terminate. This may be used by editors or other programs to control the debugger.\n\n"));
    }break;
  }
  
  scratch_end(scratch);
}
