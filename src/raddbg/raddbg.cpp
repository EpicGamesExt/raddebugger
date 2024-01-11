// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Settings

#define RADDBG_VERSION_MAJOR 0
#define RADDBG_VERSION_MINOR 9
#define RADDBG_VERSION_PATCH 0
#define RADDBG_VERSION_STRING_LITERAL Stringify(RADDBG_VERSION_MAJOR) "." Stringify(RADDBG_VERSION_MINOR) "." Stringify(RADDBG_VERSION_PATCH)
#if defined(NDEBUG)
# define RADDBG_TITLE_STRING_LITERAL "The RAD Debugger (" RADDBG_VERSION_STRING_LITERAL " ALPHA) - " __DATE__ ""
#else
# define RADDBG_TITLE_STRING_LITERAL "The RAD Debugger (" RADDBG_VERSION_STRING_LITERAL " ALPHA) - " __DATE__ " [Debug]"
#endif

#define ENABLE_DEV 1
#define DE2CTRL 1

////////////////////////////////
//~ rjf: 2024/1 tasks
//
// [ ] @bug bug notes from casey
//  [ ] ** If you put a full path to a TTF font into the code_font/main_font
//      variables of the config file, it continually rewrites it each time you
//      launch. The first time you launch, with your hand-edited font path, it
//      works correctly and the font loads, but it rewrites it from an absolute
//      path to a relative path. The second time you launch, with the relative
//      path, it doesn't work (you get no text at all), and it rewrites it from
//      relative back to absolute, but to the wrong path (prepending
//      C:/users/casey/AppData/ to the previous path, even though that was not
//      at all where the font ever was) The font path will now remain "stable"
//      in the sense that it won't rewrite it anymore. But you cannot use the
//      debugger because it's the wrong font path, so you get no text.
//  [x] When I click in the theme window on something like "Code (Meta)", it
//      opens up a tiny window, which only shows a tiny bit of what I imagine
//      is the entire color picker? It makes the picker unusable because more
//      of the UI for it is clipped and cannot be accessed.
//  [x] Clicking anywhere in the color picker that isn't a button closes the
//      color picker for some reason?
//  [ ] In a "hover watch" (where you hover over a variable and it shows a pop-
//      up watch window), if you expand an item near the bottom of the listing,
//      it will be clipped to the bottom of the listing instead of showing the
//      actual items (ie., it doesn't resize the listing based on what's
//      actually visible)
//
// [ ] @bug @feature @cleanup general feedback from casey
//  [x] ** I don't like how panels get dimmer when they are not active, because
//      it makes them harder for me to read. Is there a way to turn this off?
//  [ ] ** I couldn't figure out how to really view threads in the debugger.
//      The only place I found a thread list was in "The Scheduler", but it
//      only lists threads by ID, which is hard to use. I can hover over them
//      to get the stack, which helps, but it would be much nicer if the top
//      function was displayed in the window by default next to the thread.
//  [ ] ** It would be nice if thread listings displayed the name of the
//      thread, instead of just the ID.
//  [ ] ** Scrollbars are barely visible for me, for some reason. I could not
//      find anything in the theme that would fill them with a solid, bright
//      color. Instead they are just a thin outline and the same color as the
//      scroll bar background.
//  [x] It seems like the code is compiled as a console app for some reason, so
//      when you run it, it locks up your console. I worked around this by
//      making a bat file that does a "start" of it, but, it seems like this
//      would not be what people would expect (at least on Windows?)
//  [ ] Dragging a window tab (like Locals or Registers or whatnot) and
//      canceling with ESC should revert the window tab to where it was.
//      Currently, it leaves the window tab reordered if you dragged over its
//      window and shuffled its position.
//  [ ] Many of the UI elements, like the menus, would like better if they had
//      a little bit of margin. Having the text right next to the edges, and
//      with no line spacing, makes it harder to read things quickly.
//  [ ] Menus take too long to show up. I would prefer it if they were instant.
//      The animation doesn't really provide any useful cues, since I know
//      where the menu came from.
//  [ ] Theme window should include font scaling. I was able to find the
//      command for increasing the font scale, but I imagine most people
//      wouldn't think to look there.
//  [ ] The way the "commands" view worked was idiosyncratic. All the other
//      views stay up, but that one goes away whenever I select a command for
//      some reason.
//  [ ] Also, I could not move the commands window anywhere AFAICT. It seems
//      to just pop up over whatever window I currently have selected. This
//      would make sense for a hotkey (which I assume is the way it was
//      designed), but it seems like it should be permanent if you can select
//      it from the View menu.
//  [ ] If the command window is not wide enough, you cannot read the
//      description of a command because it doesn't word-wrap, nor can you
//      hover over it to get the description in a tooltip (AFAICT).
//  [ ] It'd be nice to have a "goto byte" option for source views, for jumping
//      to error messages that are byte-based instead of line-based.
//  [ ] Pressing the left mouse button on the menu bar and dragging does not
//      move through the menus as expected - instead, it opens the one you
//      clicked down on, then does nothing until you release, at which point it
//      opens the menu you released on.
//  [ ] Similarly, pressing the left mouse button on a menu and dragging to an
//      item, then releasing, does not trigger that item as expected. Instead,
//      it is a nop, and it waits for you to click again on the item.
//  [ ] I was a little confused about what a profile file was. I understood
//      what the user file was, but the profile file sounded like it should
//      perhaps be per-project, yet it sounded like it was meant to be somewhat
//      global? I don't have any feedback here because it probably will make
//      sense once I use the debugger more, but I just thought I'd make a note
//      to say that I was confused about it after reading the manual, so
//      perhaps you could elaborate a little more on it in there.
//  [ ] I found the "non-windowness" of the source view area confusing. Since
//      everything else is a tab, it was weird that it was a "phantom non-tab
//      window" that gets replaced by an automatically opened tab once you step.
//  [ ] Working with panels felt cumbersome. I couldn't figure out any way to
//      quickly arrange the display without manually selecting "split panel"
//      and "close panel" and stuff from the menu, which took a long time.
//  [ ] I found the "context menu" convention to be confusing. For example, if
//      I left-click on a tab, it selects the tab. If I right-click on a tab,
//      it opens the context menu. However, if I left-click on a module, it
//      opens the context window. It seems like maybe menus should be right,
//      and left should do the default action, more consistently?
//  [ ] I found the "drill down" convention to be confusing, too. For example,
//      if I click on a target, it opens a new tab with that target in it. By
//      contrast, if I click on "browse" in the module window, it replaces that
//      window temporarily with a browsing window.
//  [ ] More tooltips would be helpful. For example, I don't know what the
//      "rotation arrow" icon next to executables means in the "This PC" window
//  [ ] Hovering over disassembly highlights blocks of instructions, which I
//      assume correspond to source lines. But perhaps it should also highlight
//      the source lines? The inverse hover works (you hover over source, and
//      it highlights ASM), but ASM->source doesn't.
//  [ ] It seems like clicking on a breakpoint red circle should perhaps
//      disable the breakpoint, rather than delete it? Since breakpoints can
//      have a fair bit of state, it seems like it might be annoying to have it
//      all be deleted if you accidentally click. Unless you are planning on
//      adding undo? Actually, that's probably something you should do: keep a
//      breakpoint history that is in the breakpoint window that you can open
//      up when you want, and it just has a stack of all your recently deleted
//      breakpoints? That solves the problem better.
//  [ ] I had to go into the user file to change the font. That should probably
//      be in the theme window?
//  [ ] Launching the debugger with an invalid code_font/main_font name doesn't
//      have any fallback, so you just get no text at all. Probably should use
//      a fallback font when font loading fails
//  [ ] Setting the code_font/main_font values to a font name doesn't work.
//      Should probably make note that you have to set it to a path to a TTF,
//      since that's not normally how Windows fonts work.
//  [ ] Having inactive tabs use smaller fonts doesn't work very well for
//      readability. I feel like just normal->bold is a better choice than
//      small font->large font.
//  [ ] The hex format for color values in the config file was a real
//      mindbender. It's prefixed with "0x", so I was assuming it was either
//      Windows Big Endian (0xAARRGGBB) or Mac Little Endian (0xAABBGGRR). To
//      my surprise, it was neither - it was actually web format (RRGGBBAA),
//      which I was not expecting because that is normally written with a
//      number sign (#AARRGGBB) not an 0x.
//  [ ] Clicking on either side of a scroll bar is idiosyncratic. Normally,
//      that is "page up" / "page down", but here it is "smooth scroll upward"
//      / "smooth scroll downward" for some reason?
//  [ ] Hitting ESC during a color picker drag should abort the color picking
//      and revert to the previous color. Currently, it just accepts the last
//      drag result as the new color.
//  [ ] It was not clear to me why a small "tab picker" appeared when I got to
//      a certain number of tabs. It seemed to appear even if the tabs were
//      quite large, and there was no need to a drop-down menu to pick them. It
//      feels like either it should always be there, or it should only show up
//      if at least one tab gets small enough to have its name cut off?
//  [ ] It feels like "expansion" icons should only show next to things that
//      can actually be expanded.
//  [ ] It wasn't clear to me how you save a user or profile file. I can see
//      how to load them, but not how you save them. Obviously I can just copy
//      the files myself in the shell, but it seemed weird that there was no
//      "save" option in the menus.
//
//*[ ] @cleanup @feature double & triple click select in source views
//*[ ] @bug disasm animation & go-to-address
//
//*[ ] @bug table column boundaries should be checked against *AFTER* table contents, not before
//*[ ] @cleanup @feature autocomplete lister should respect position in edited expression, tabbing through should autocomplete but not exit, etc.
//*[ ] @cleanup @feature figure out 'watch window state' question, and how watch windows relate to targets, entities, & so on
//*[ ] @feature undo/redo
//*[ ] @feature proper "go back" + "go forward" history navigations
//
// [ ] @bug view-snapping in scroll-lists, accounting for mapping between visual positions & logical positions (variably sized rows in watch, table headers, etc.)
// [ ] @bug selected frame should be keyed by run_idx or something so that it can gracefully reset to the top frame when running
//
// [ ] @cleanup collapse DF_CfgNodes into just being MD trees, find another way to encode config source - don't need it at every node
//
// [ ] @feature fancy "escape hatch" view rules
//  [ ] `text[:lang]` - interpret memory as text, in lang `lang`
//  [ ] `disasm:arch` - interpret memory as machine code for isa `arch`
//  [ ] `memory` - view memory in usual memory hex-editor view
//  NOTE(rjf): When the visualization system is solid, layers like dasm, txti, and so on
//  can be dispensed with, as things like the source view, disasm view, or memory view will
//  simply be specializations of the general purpose viz system.
//
// [ ] @feature view rule hook for standalone visualization ui, granted its own tab
// [ ] @feature hovering truncated text in UI for some amount of time -> show tooltip with full text
//
// [ ] @cleanup straighten out index/number space & types & terminology for scroll lists
// [ ] @cleanup simplification pass over eval visualization pipeline & types, including view rule hooks
// [ ] @cleanup naming pass over eval visualization part of the frontend, "blocks" vs. "canvas" vs. "expansion" - etc.
//
// [ ] @feature disasm keyboard navigation & copy/paste
// [ ] @feature debug info overrides (both path-based AND module-based)
//
// [ ] @polish globally disable/configure default view rule-like things (string viz for u8s in particular)
// [ ] @polish configure tab size
//
// [ ] @polish @feature ui for dragging tab -> bundling panel split options
// [ ] @polish @feature visualize mismatched source code and debug info
// [ ] @polish @feature visualize remapped files (via path map)
// [ ] @polish @feature run-to-line needs to work if no processes are running - place temp bp, attach "die on hit" flag or something like that?
//
// [ ] @feature processor/data breakpoints
// [ ] @feature automatically snap to search matches when searching source files
// [ ] @feature entity views: filtering & reordering
//
// [ ] @cleanup central worker thread pool - eliminate per-layer thread pools
//
// [ ] @feature search-in-all-files
// [ ] @feature memory view mutation controls
// [ ] @feature memory view user-made annotations

////////////////////////////////
//~ rjf: 2024/2 tasks
//
// [ ] @perf ue/chromium testing
// [ ] @perf converter perf
// [ ] @perf frontend perf
//
// [ ] @feature types -> auto view rules (don't statefully fill view rules given types, just query if no other view rule is present, & autofill when editing)
// [ ] @feature eval system -> somehow evaluate breakpoint hit counts? "meta" variables?
// [ ] @feature watch window labels
// [ ] @feature scheduler -> thread grid view?
// [ ] @feature global entry point overrides (settings)
//
// [ ] @feature disasm view improvement features
//  [ ] interleaved src/dasm view
//  [ ] visualize jump destinations in disasm
//
// [ ] @feature eval ui improvement features
//  [ ] serializing eval view maps
//  [ ] view rule editors in hover-eval
//  [ ] view rule hook coverage
//   [ ] `each:(expr addition)` - apply some additional expression to all elements in an array/linked list would be useful to look at only a subset of an array of complex structs
//   [ ] `slider:(min max)` view rule
//   [ ] `v2f32` view rule
//   [ ] `v3` view rule
//   [ ] `quat` view rule
//   [ ] `matrix` view rule
//   [ ] `audio` waveform view rule
//  [ ] smart scopes - expression operators for "grab me the first type X"
//  [ ] "pinning" watch expressions, to attach it to a particular ctrl_ctx
//
// [ ] @feature header file for target -> debugger communication; printf, log, etc.
// [ ] @feature just-in-time debugging
// [ ] @feature step-out-of-loop

////////////////////////////////
//~ rjf: Unsorted User Notes
//
//-[ ] jeff notes
//  [ ] keyboard controls & copy/paste in disasm view
//  [ ] disasm animation unexpected and/or annoying
//
//-[ ] notes from allen
//  [ ] 'browse' in modules debug info map editor
//  [ ] debug info map revisit... ctrl thread needs to know string -> string, not just handle -> string
//  [ ] panel-split options in right-click menu. maybe just do the full
//      'contextual ctx menu stack' thing here? then ctx menus buried deep in
//      the callstack still gather all the right stuff...
//  [ ] ui color pallette inspector/editor - function which takes ui_box -> theme_color
//  [ ] watch-window-wide view rule
//
//-[ ] short-term major issues from martins
//  [ ] F10/F5 is not stepping over line, if line invokes macro with bunch of code (if checks, functions
//      calls) then debugger thinks there is a breakpoint on every condition? or every function call? so
//      I hits my breakpoint multiple times when I just want to get over it
//
//-[ ] short-term minor issues from martins
//  [ ]  can it ignore stepping into _RTC_CheckStackVars generated functions?
//  [ ]  mouse back button should make view to go back after I double clicked on function to open it
//  [ ]  middle mouse button on tab should close it
//  [ ]  pressing random keyboard keys in source code advances text cursor like you were inputting text, very strange.
//  [ ]  Alt+8 to switch to disassembly would be nice (regardless on which panel was previous, don't want to use ctrl+, multiple times)
//       Alt+8 for disasm and Alt+6 for memory view are shortcuts I often use in VS
//  [ ]  what's up with decimal number coloring where every group of 3 are in different color? can I turn it off? And why sometimes digits in number start with brighter color, but sometimes with darker - shouldn't it always have the same color ordering?
//  [ ]  it would be nice to have "show in explorer" for right click on source file tab (opens explorer & selects the file)
//  [ ]  it would be nice if Alt+o in source file would switch between .h and .c/cpp file (just look for same name in same folder)
//  [ ]  in watch window when I enter some new expression and then click mouse away from cell, then it should behave the same as if I pressed enter. Currently it does the same as if I have pressed esc and I have lost my expression
//  [ ]  navigation in watch window cells (not text editing, but which cell is selected) is bad - home/end/pgup/pgdown buttons navigate not where I expect (and with ctrl combination too)
//  [ ]  for big source files the scrollbar element for active position is a bit to small - maybe have min size it never goes below (like 50px or maybe exactly 1 line of source code in height)
//  [ ]  default font size is too small for me - not only source code, but menus/tab/watch names (which don't resize). Maybe you could query Windows for initial font size?
//  [ ]  zooming behaves very strangely - sometimes it zooms source code, sometimes both source code and menu/tab/watch font size, sometimes just menu/tab/watch font size not source size.
//  [ ]  icon fonts glyphs sometimes disappear for specific font size, but they reappear if you go +1 higher or -1 lower. Mostly red triangle in watch values for "unknown identifier". But also yellow arrow in call stack disappears if font size gets too large.
//  [ ]  undo close tab would be nice. If not for everything, then at least just for source files
//
//-[ ] long-term future notes from martins
//  [ ] core dump saving/loading
//  [ ] parallel call stacks view
//  [ ] parallel watch view
//  [ ] mixed native/interpreted/jit debugging
//      - it seems python has a top-level linked list of interpreter states,
//        which should allow the debugger to map native callstacks to python
//        code

////////////////////////////////
//~ rjf: Long-Term Deferred Polish & Improvements
//
// [ ] fancy string runs can include "weakness" information for text truncation... can prioritize certain parts of strings to be truncated before others. would be good for e.g. the middle of a path
// [ ] ui code maintenance, simplification, design, & robustness pass
//  [ ] page-up & page-down correct handling in keyboard nav
//  [ ] collapse context menus & command lister into same codepaths. filter by context. parameterize by context.
//  [ ] collapse text cells & command lister & etc. into same codepath (?)
//  [ ] nested context menus
//  [ ] unified top-level cursor/typing/lister helper
//  [ ] font selection lister
// [ ] font cache eviction (both for font tags, closing fp handles, and rasterizations)
// [ ] frontend speedup opportunities
//  [ ] tables in UI -> currently building per-row, could probably cut down on # of boxes and # of draws by doing per-column in some cases?
//  [ ] font cache layer -> can probably cache (string*font*size) -> (run) too (not just rasterization)... would save a *lot*, there is a ton of work just in looking up & stitching stuff repeatedly
//  [ ] convert UI layout pass to not be naive recursive version
//  [ ] (big change) parallelize window ui build codepaths per-panel

////////////////////////////////
//~ rjf: Completed Tasks
//
// [x] adding watches to source locations
// [x] tab overflow buttons
// [x] "working set" of targets, just used 'enabled' slot on target entities
// [x] convert hover-eval ui to a frontend-wide global ui concept, so that we can have watch-hover trees from anywhere
// [x] annotate code slices with relevant watches
// [x] ability to duplicate targets easily
// [x] ability to label targets separately from their EXE
//
// [x] txti layer needs lexing
// [x] dbgi autoconversion layer needs to work reliably
// [x] new dbgi system on frontend
// [x] new bini system on frontend
// [x] new dbgi system needs in-parallel typegraph builders
// [x] eval is busted
// [x] fail or debug non-text
// [x] clean loading
// [x] watch pins
// [x] snapping should still work if a file is loading while the snap first occurs
// [x] keyboard usage (set-next-statement, run-to-line, etc.)
// [x] go-to-name (both file & symbol)
//
// [x] in-memory disassembly view
// [x] visualize conversion tasks as they occur
// [x] raddbg.exe self-sufficiency => built-in converter execution mode
// [x] move path-mapping fallback from pending-entity into code view
// [x] per-target entry point overrides
// [x] logic on when double click on callstack entry goes to source or disassembly is very confusing,
//     because it seems it matters whether disassembly tab is in same panel as source, or is placed in
//     different panel in VS double clicking on stack stays in disassembly if disassembly tab is currently
//     active. Otherwise if source is active, it goes to source code (regardless if disasm is open/visible
//     or not)
// [x] global variable (just regular C++ bool in the same file) does not show value when hovering over source
//     code, or when put into watch window.
//
// [x] debugger ui thread needs wakeup when a debug event is hit
// [x] ctrl thread needs to re-resolve breakpoints as modules/threads come in during a run
// [x] automatically pull in members from containing struct in member functions
//     when looking up locals
// [x] iron out weird state machine bugs with hover-eval
//
// [x] thread-local eval
// [x] tabs in source code render as squares
//
// [x] breakpoint stop-conditions
// [x] first-chance exception hitting (d3d11 as an example offender)
// [x] soft-halt refresh
//
// [x] conditional breakpoints cannot be submitted if they don't compile
//     need to (a) visualize and (b) equip the ctrl thread with a 'halt on
//     new debug info feature', which can be done repeatedly until something
//     compiles (?)
//
// [x] outputdebugstring logs
//
// [x] memory view
//
// [x]  when application starts then the focus is on cmd.exe window of application not
//      debugger. This means if I press F11 to step into main(), the application cmd.exe will go fullscreen
//      hiding debugger (F11 key for ConEmu I use makes it go fullscreen)
// [x]  I'd prefer if ctrl+click on word would go to function definition, not double click
//      double clicking in source code should select word - which easy way to double click and ctrl+c to copy
//      it, pretty much standard thing in any text editorr/viewer and triple click for selecting whole line
//  [x] command line DF_CfgSrc -> explicitly visualize as temporary, provide UI
//      path to make permanent (in either user or profile?)
//
// [x] memory view annotations: bytes in a range of memory actually have a
//     *stack* of possible interpretations, for any particular low-level
//     representation (assuming 1 is fine for this case). a U32 member in
//     a local struct on the stack has 3 layers: U32 -> local -> stack.
//     each of these are useful information about that U32's range of bytes.
//     so, the memory view annotation system ought to be expanded to support-
//     ing stacks of annotations, and gracefully visualizing multiple nested
//     ones. after that is supported, we can use the type info to derive the
//     lowest level representation of some bytes. each stack can only have
//     one low-level type. that low-level type, then, can be used to directly
//     interpret the bytes - the others can be used to show the "conceptual
//     stack".
//
// [x] entity tree ui replacement (scheduler, modules, breakpoints, pins)
// [x] theme menu
// [x] complete transition to UI_ScrollPt coordinate space scrolling - eliminate
//     old scrolls & scroll regions
// [x] settings menu
// [x] exception filter settings & controls in ctrl layer
//
// [x] @polish @cleanup convert eval/watch to scroll list
// [x] @feature memory view keyboard navigation
// [x] @bug fix general ui line edit editing rules - conflicting with navigation, etc.
// [x] @polish have expander space even if not used in watch window
// [x] @bug references do not expand properly
// [x] @bug panel deletion improper size bug
// [x] @bug panel serialization/deserialization bug?
// [x] unfinished char/string literal lexing
//
//  [x] C++ problems in watch window: evaluating "this" shows no members. The type seems correct reference
//      to struct (which is local variable) does not show any members, like I have "tm_mem_zone_rt& z = ..."
//      in source code, and doing "z" produces no children, nor does "&z" but then at least it shows correct
//      address as value. if I ask for member of this when currently inside of member function (like "m_zone_stack")
//      it shows "unknown identifier m_zone_stack" but "this->m_zone_stack" works fine!
//  [x]  if I write x'z in watch window and press enter, it only shows x and loses 'z - it does not show it, but when I edit cell then 'z comes back. It pretends I entered just x and shows x value.
//  [x]  it seems the glyph advance for default font size is kind of wrong, as characters are a blitted a bit over top of each other
//
// [x] @bug deleting a watch tree while it is expanded causes cursor to go back to first row - cannot simply increment cursor. maybe keep in same spot, but rebuild viz blocks?
// [x] @bug do not squish partially-cut-off view rule block uis
// [x] @feature "solo step", or "solo mode" freeze-all-unselected-threads-on-step-commands
// [x] @feature typing autocomplete lister
// [x] @feature directional navigation of panel focus
// [x] @cleanup @feature cache eviction in texture cache & hash store
//
//- 2023/12/7
//
// [x] @bug hash store cache eviction can only work if user never blindly tries to go from hash -> data, because
//          they must be able to retry... hmm...
// [x] txt cell revamp. keyboard focus in both default & non w/ multiple options, helper lister, etc.
// [x] `bitmap:(w:width, h:height, [fmt:fmt])` - interpret memory as raw bitmap data
// [x] `geo:n[ topology stride]` - interpret memory as geometry
//  [x] cursor helper -> upgraded txt cell. classify inputs & show dropdowns (locals, globals, types, view rules, etc.)
// [x] @bug page-up and page-down in src view, when near the end of file
//
//- 2023/12/8
//
// [x] @bug parse `unsigned int` correctly in eval parser
// [x] @bug ., + operators should work on registers
// [x] @bug straighten out register eval problems
// [x] @cleanup finish ui_em transition
//
//- 2023/12/22
//
// [x] @bug set-bp-while-running seems to not resume after soft-halt, might be a soft-halt bug
// [x] @bug weird view snapping in watch scrolling down
//
//- 2024/01/10
//
// [x] @feature allow `,count`, `,x`, `,b` style watch window expression extensions, which add to view rule, for VS-like behavior fastpaths

////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "mdesk/mdesk.h"
#include "hash_store/hash_store.h"
#include "text_cache/text_cache.h"
#include "path/path.h"
#include "txti/txti.h"
#include "coff/coff.h"
#include "pe/pe.h"
#include "raddbg_format/raddbg_format.h"
#include "raddbg_format/raddbg_format_parse.h"
#include "raddbg_cons/raddbg_cons.h"
#include "raddbg_convert/pdb/raddbg_coff.h"
#include "raddbg_convert/pdb/raddbg_codeview.h"
#include "raddbg_convert/pdb/raddbg_msf.h"
#include "raddbg_convert/pdb/raddbg_pdb.h"
#include "raddbg_convert/pdb/raddbg_coff_conversion.h"
#include "raddbg_convert/pdb/raddbg_codeview_conversion.h"
#include "raddbg_convert/pdb/raddbg_from_pdb.h"
#include "raddbg_convert/pdb/raddbg_codeview_stringize.h"
#include "raddbg_convert/pdb/raddbg_pdb_stringize.h"
#include "regs/regs.h"
#include "regs/raddbg/regs_raddbg.h"
#include "type_graph/type_graph.h"
#include "dbgi/dbgi.h"
#include "demon/demon_inc.h"
#include "eval/eval_compiler.h"
#include "eval/eval_machine.h"
#include "eval/eval_parser.h"
#include "unwind/unwind.h"
#include "ctrl/ctrl_inc.h"
#include "dasm/dasm.h"
#include "font_provider/font_provider_inc.h"
#include "render/render_inc.h"
#include "texture_cache/texture_cache.h"
#include "geo_cache/geo_cache.h"
#include "font_cache/font_cache.h"
#include "draw/draw.h"
#include "ui/ui_inc.h"
#include "df/df_inc.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "mdesk/mdesk.c"
#include "hash_store/hash_store.c"
#include "text_cache/text_cache.c"
#include "path/path.c"
#include "txti/txti.c"
#include "coff/coff.c"
#include "pe/pe.c"
#include "raddbg_format/raddbg_format.c"
#include "raddbg_format/raddbg_format_parse.c"
#include "raddbg_cons/raddbg_cons.c"
#include "raddbg_convert/pdb/raddbg_msf.c"
#include "raddbg_convert/pdb/raddbg_codeview.c"
#include "raddbg_convert/pdb/raddbg_pdb.c"
#include "raddbg_convert/pdb/raddbg_coff_conversion.c"
#include "raddbg_convert/pdb/raddbg_codeview_conversion.c"
#include "raddbg_convert/pdb/raddbg_codeview_stringize.c"
#include "raddbg_convert/pdb/raddbg_pdb_stringize.c"
#include "raddbg_convert/pdb/raddbg_from_pdb.c"
#include "regs/regs.c"
#include "regs/raddbg/regs_raddbg.c"
#include "type_graph/type_graph.c"
#include "dbgi/dbgi.c"
#include "demon/demon_inc.c"
#include "eval/eval_compiler.c"
#include "eval/eval_machine.c"
#include "eval/eval_parser.c"
#include "unwind/unwind.c"
#include "ctrl/ctrl_inc.c"
#include "dasm/dasm.c"
#include "font_provider/font_provider_inc.c"
#include "render/render_inc.c"
#include "texture_cache/texture_cache.c"
#include "geo_cache/geo_cache.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"
#include "ui/ui_inc.c"
#include "df/df_inc.c"

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
//~ rjf: Top-Level Execution Globals

#define IPC_SHARED_MEMORY_BUFFER_SIZE MB(16)
StaticAssert(IPC_SHARED_MEMORY_BUFFER_SIZE > sizeof(IPCInfo), ipc_buffer_size_requirement);
read_only global String8 ipc_shared_memory_name = str8_lit_comp("_raddbg_ipc_shared_memory_");
read_only global String8 ipc_semaphore_name = str8_lit_comp("_raddbg_ipc_semaphore_");

////////////////////////////////
//~ rjf: Frontend Entry Points

internal void
update_and_render(OS_Handle repaint_window_handle, void *user_data)
{
  ProfTick(0);
  ProfBeginFunction();
  Temp scratch = scratch_begin(0, 0);
  
  //- rjf: tick cache layers
  txt_user_clock_tick();
  geo_user_clock_tick();
  tex_user_clock_tick();
  
  //- rjf: pick delta-time
  // TODO(rjf): maximize, given all windows and their monitors
  F32 dt = 1.f/os_default_refresh_rate();
  
  //- rjf: get events from the OS
  OS_EventList events = {0};
  if(os_handle_match(repaint_window_handle, os_handle_zero()))
  {
    events = os_get_events(scratch.arena, df_gfx_state->num_frames_requested == 0);
  }
  
  //- rjf: bind change
  if(df_gfx_state->bind_change_active)
  {
    if(os_key_press(&events, os_handle_zero(), 0, OS_Key_Esc))
    {
      df_gfx_state->bind_change_active = 0;
    }
    if(os_key_press(&events, os_handle_zero(), 0, OS_Key_Delete))
    {
      df_unbind_spec(df_gfx_state->bind_change_cmd_spec, df_gfx_state->bind_change_binding);
      df_gfx_state->bind_change_active = 0;
      DF_CmdParams p = df_cmd_params_from_gfx();
      df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(df_g_cfg_src_write_cmd_kind_table[DF_CfgSrc_User]));
    }
    for(OS_Event *event = events.first, *next = 0; event != 0; event = next)
    {
      if(event->kind == OS_EventKind_Press &&
         event->key != OS_Key_Esc &&
         event->key != OS_Key_Return &&
         event->key != OS_Key_Backspace &&
         event->key != OS_Key_Delete &&
         event->key != OS_Key_LeftMouseButton &&
         event->key != OS_Key_RightMouseButton &&
         event->key != OS_Key_Ctrl &&
         event->key != OS_Key_Alt &&
         event->key != OS_Key_Shift)
      {
        df_gfx_state->bind_change_active = 0;
        DF_Binding binding = zero_struct;
        {
          binding.key = event->key;
          binding.flags = event->flags;
        }
        df_unbind_spec(df_gfx_state->bind_change_cmd_spec, df_gfx_state->bind_change_binding);
        df_bind_spec(df_gfx_state->bind_change_cmd_spec, binding);
        U32 codepoint = os_codepoint_from_event_flags_and_key(event->flags, event->key);
        os_text(&events, os_handle_zero(), codepoint);
        os_eat_event(&events, event);
        DF_CmdParams p = df_cmd_params_from_gfx();
        df_push_cmd__root(&p, df_cmd_spec_from_core_cmd_kind(df_g_cfg_src_write_cmd_kind_table[DF_CfgSrc_User]));
        break;
      }
    }
  }
  
  //- rjf: take hotkeys
  {
    for(OS_Event *event = events.first, *next = 0;
        event != 0;
        event = next)
    {
      next = event->next;
      DF_Window *window = df_window_from_os_handle(event->window);
      DF_CmdParams params = window ? df_cmd_params_from_window(window) : df_cmd_params_from_gfx();
      if(event->kind == OS_EventKind_Press)
      {
        DF_Binding binding = {event->key, event->flags};
        DF_CmdSpecList spec_candidates = df_cmd_spec_list_from_binding(scratch.arena, binding);
        if(spec_candidates.first != 0 && !df_cmd_spec_is_nil(spec_candidates.first->spec))
        {
          DF_CmdSpec *spec = spec_candidates.first->spec;
          params.cmd_spec = spec;
          df_cmd_params_mark_slot(&params, DF_CmdParamSlot_CmdSpec);
          U32 hit_char = os_codepoint_from_event_flags_and_key(event->flags, event->key);
          os_eat_event(&events, event);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CommandFastPath));
          if(event->flags & OS_EventFlag_Alt)
          {
            window->menu_bar_focus_press_started = 0;
          }
        }
        df_gfx_request_frame();
      }
      else if(event->kind == OS_EventKind_Text)
      {
        String32 insertion32 = str32(&event->character, 1);
        String8 insertion8 = str8_from_32(scratch.arena, insertion32);
        DF_CmdSpec *spec = df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_InsertText);
        params.string = insertion8;
        df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
        df_push_cmd__root(&params, spec);
        df_gfx_request_frame();
      }
    }
  }
  
  //- rjf: menu bar focus
  {
    for(OS_Event *event = events.first, *next = 0; event != 0; event = next)
    {
      next = event->next;
      DF_Window *ws = df_window_from_os_handle(event->window);
      if(ws == 0)
      {
        continue;
      }
      if(event->kind == OS_EventKind_Press && event->key == OS_Key_Alt && event->is_repeat == 0)
      {
        ws->menu_bar_focused_on_press = ws->menu_bar_focused;
        ws->menu_bar_key_held = 1;
        ws->menu_bar_focus_press_started = 1;
      }
      if(event->kind == OS_EventKind_Release && event->key == OS_Key_Alt && event->is_repeat == 0)
      {
        ws->menu_bar_key_held = 0;
      }
      if(ws->menu_bar_focused && event->kind == OS_EventKind_Press && event->key == OS_Key_Alt && event->is_repeat == 0)
      {
        os_eat_event(&events, event);
        ws->menu_bar_focused = 0;
      }
      else if(ws->menu_bar_focus_press_started && !ws->menu_bar_focused && event->kind == OS_EventKind_Release && event->key == OS_Key_Alt && event->is_repeat == 0)
      {
        os_eat_event(&events, event);
        ws->menu_bar_focused = !ws->menu_bar_focused_on_press;
        ws->menu_bar_focus_press_started = 0;
      }
      else if(event->kind == OS_EventKind_Press && event->key == OS_Key_Esc && ws->menu_bar_focused && !ui_any_ctx_menu_is_open())
      {
        os_eat_event(&events, event);
        ws->menu_bar_focused = 0;
      }
    }
  }
  
  //- rjf: gather root-level commands
  DF_CmdList cmds = df_core_gather_root_cmds(scratch.arena);
  
  //- rjf: begin frame
  df_core_begin_frame(scratch.arena, &cmds, dt);
  df_gfx_begin_frame(scratch.arena, &cmds);
  
  //- rjf: queue drop for drag/drop
  if(df_drag_is_active())
  {
    for(OS_Event *event = events.first; event != 0; event = event->next)
    {
      if(event->kind == OS_EventKind_Release && event->key == OS_Key_LeftMouseButton)
      {
        df_queue_drag_drop();
        break;
      }
    }
  }
  
  //- rjf: auto-focus moused-over windows while dragging
  if(df_drag_is_active())
  {
    B32 over_focused_window = 0;
    {
      for(DF_Window *window = df_gfx_state->first_window; window != 0; window = window->next)
      {
        Vec2F32 mouse = os_mouse_from_window(window->os);
        Rng2F32 rect = os_client_rect_from_window(window->os);
        if(os_window_is_focused(window->os) && contains_2f32(rect, mouse))
        {
          over_focused_window = 1;
          break;
        }
      }
    }
    if(!over_focused_window)
    {
      for(DF_Window *window = df_gfx_state->first_window; window != 0; window = window->next)
      {
        Vec2F32 mouse = os_mouse_from_window(window->os);
        Rng2F32 rect = os_client_rect_from_window(window->os);
        if(!os_window_is_focused(window->os) && contains_2f32(rect, mouse))
        {
          os_window_focus(window->os);
          break;
        }
      }
    }
  }
  
  //- rjf: update & render
  {
    d_begin_frame();
    for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
    {
      df_window_update_and_render(scratch.arena, &events, w, &cmds);
    }
  }
  
  //- rjf: end frontend frame, send signals, etc.
  df_gfx_end_frame();
  df_core_end_frame();
  
  //- rjf: submit rendering to all windows
  {
    r_begin_frame();
    for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
    {
      r_window_begin_frame(w->os, w->r);
      d_submit_bucket(w->os, w->r, w->draw_bucket);
      r_window_end_frame(w->os, w->r);
    }
    r_end_frame();
  }
  
  //- rjf: take window closing events
  for(OS_Event *e = events.first; e; e = e->next)
  {
    if(e->kind == OS_EventKind_WindowClose)
    {
      for(DF_Window *w = df_gfx_state->first_window; w != 0; w = w->next)
      {
        if(os_handle_match(w->os, e->window))
        {
          DF_CmdParams params = df_cmd_params_from_window(w);
          df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_CloseWindow));
          break;
        }
      }
    }
  }
  
  scratch_end(scratch);
  ProfEnd();
}

internal CTRL_WAKEUP_FUNCTION_DEF(wakeup_hook)
{
  os_send_wakeup_event();
}

internal void
entry_point(int argc, char **argv)
{
  Temp scratch = scratch_begin(0, 0);
#if PROFILE_TELEMETRY
  local_persist U8 tm_data[MB(64)];
  tmLoadLibrary(TM_RELEASE);
  tmSetMaxThreadCount(1024);
  tmInitialize(sizeof(tm_data), (char *)tm_data);
#endif
  ThreadName("[main]");
  
  //- rjf: initialize basic dependencies
  os_init(argc, argv);
  
  //- rjf: parse command line arguments
  CmdLine cmdln = cmd_line_from_string_list(scratch.arena, os_get_command_line_arguments());
  ExecMode exec_mode = ExecMode_Normal;
  String8 user_cfg_path = str8_lit("");
  String8 profile_cfg_path = str8_lit("");
  B32 capture = 0;
  B32 auto_run = 0;
  B32 auto_step = 0;
  B32 jit_attach = 0;
  U64 jit_pid = 0;
  U64 jit_code = 0;
  U64 jit_addr = 0;
  {
    if(cmd_line_has_flag(&cmdln, str8_lit("ipc")))
    {
      exec_mode = ExecMode_IPCSender;
    }
    else if(cmd_line_has_flag(&cmdln, str8_lit("convert")))
    {
      exec_mode = ExecMode_Converter;
    }
    else if(cmd_line_has_flag(&cmdln, str8_lit("?")) ||
            cmd_line_has_flag(&cmdln, str8_lit("help")))
    {
      exec_mode = ExecMode_Help;
    }
    user_cfg_path = cmd_line_string(&cmdln, str8_lit("user"));
    profile_cfg_path = cmd_line_string(&cmdln, str8_lit("profile"));
    capture = cmd_line_has_flag(&cmdln, str8_lit("capture"));
    auto_run = cmd_line_has_flag(&cmdln, str8_lit("auto_run"));
    auto_step = cmd_line_has_flag(&cmdln, str8_lit("auto_step"));
    String8 jit_pid_string = {0};
    String8 jit_code_string = {0};
    String8 jit_addr_string = {0};
    jit_pid_string = cmd_line_string(&cmdln, str8_lit("jit_pid"));
    jit_code_string = cmd_line_string(&cmdln, str8_lit("jit_code"));
    jit_addr_string = cmd_line_string(&cmdln, str8_lit("jit_addr"));
    try_u64_from_str8_c_rules(jit_pid_string, &jit_pid);
    try_u64_from_str8_c_rules(jit_code_string, &jit_code);
    try_u64_from_str8_c_rules(jit_addr_string, &jit_addr);
    jit_attach = (jit_addr != 0);
  }
  
  //- rjf: auto-start capture
  if(capture)
  {
    ProfBeginCapture("raddbg");
  }
  
  //- rjf: set default user/profile paths
  {
    String8 user_program_data_path = os_string_from_system_path(scratch.arena, OS_SystemPath_UserProgramData);
    String8 user_data_folder = push_str8f(scratch.arena, "%S/%S", user_program_data_path, str8_lit("raddbg"));
    os_make_directory(user_data_folder);
    if(user_cfg_path.size == 0)
    {
      user_cfg_path = push_str8f(scratch.arena, "%S/default.raddbg_user", user_data_folder);
    }
    if(profile_cfg_path.size == 0)
    {
      profile_cfg_path = push_str8f(scratch.arena, "%S/default.raddbg_profile", user_data_folder);
    }
  }
  
  //- rjf: dispatch to top-level codepath based on execution mode
  switch(exec_mode)
  {
    //- rjf: normal execution
    default:
    case ExecMode_Normal:
    {
      //- rjf: set up shared memory for ipc
      OS_Handle ipc_shared_memory = os_shared_memory_alloc(IPC_SHARED_MEMORY_BUFFER_SIZE, ipc_shared_memory_name);
      void *ipc_shared_memory_base = os_shared_memory_view_open(ipc_shared_memory, r1u64(0, IPC_SHARED_MEMORY_BUFFER_SIZE));
      OS_Handle ipc_semaphore = os_semaphore_alloc(1, 1, ipc_semaphore_name);
      IPCInfo *ipc_info = (IPCInfo *)ipc_shared_memory_base;
      ipc_info->msg_size = 0;
      
      //- rjf: initialize stuff we depend on
      {
        hs_init();
        txt_init();
        dbgi_init();
        txti_init();
        demon_init();
        ctrl_init(wakeup_hook);
        dasm_init();
        os_graphical_init();
        fp_init();
        r_init();
        tex_init();
        geo_init();
        f_init();
        DF_StateDeltaHistory *hist = df_state_delta_history_alloc();
        df_core_init(user_cfg_path, profile_cfg_path, hist);
        df_gfx_init(update_and_render, hist);
        os_set_cursor(OS_Cursor_Pointer);
      }
      
      //- rjf: setup initial target from command line args
      {
        String8List args = cmdln.inputs;
        if(args.node_count > 0 && args.first->string.size != 0)
        {
          Temp scratch = scratch_begin(0, 0);
          DF_Entity *target = df_entity_alloc(0, df_entity_root(), DF_EntityKind_Target);
          df_entity_equip_b32(target, 1);
          df_entity_equip_cfg_src(target, DF_CfgSrc_CommandLine);
          String8List passthrough_args_list = {0};
          for(String8Node *n = args.first->next; n != 0; n = n->next)
          {
            str8_list_push(scratch.arena, &passthrough_args_list, n->string);
          }
          
          // rjf: equip exe
          if(args.first->string.size != 0)
          {
            DF_Entity *exe = df_entity_alloc(0, target, DF_EntityKind_Executable);
            df_entity_equip_name(0, exe, args.first->string);
          }
          
          // rjf: equip path
          String8 path_part_of_arg = str8_chop_last_slash(args.first->string);
          if(path_part_of_arg.size != 0)
          {
            String8 path = push_str8f(scratch.arena, "%S/", path_part_of_arg);
            DF_Entity *execution_path = df_entity_alloc(0, target, DF_EntityKind_ExecutionPath);
            df_entity_equip_name(0, execution_path, path);
          }
          
          // rjf: equip args
          StringJoin join = {str8_lit(""), str8_lit(" "), str8_lit("")};
          String8 args_str = str8_list_join(scratch.arena, &passthrough_args_list, &join);
          if(args_str.size != 0)
          {
            DF_Entity *args_entity = df_entity_alloc(0, target, DF_EntityKind_Arguments);
            df_entity_equip_name(0, args_entity, args_str);
          }
          scratch_end(scratch);
        }
      }
      
      //- rjf: main application loop
      {
        for(;;)
        {
          //- rjf: get IPC messages & dispatch ui commands from them
          {
            if(os_semaphore_take(ipc_semaphore, max_U64))
            {
              if(ipc_info->msg_size != 0)
              {
                U8 *buffer = (U8 *)(ipc_info+1);
                U64 msg_size = ipc_info->msg_size;
                String8 cmd_string = str8(buffer, msg_size);
                ipc_info->msg_size = 0;
                DF_Window *dst_window = df_gfx_state->first_window;
                for(DF_Window *window = dst_window; window != 0; window = window->next)
                {
                  if(os_window_is_focused(window->os))
                  {
                    dst_window = window;
                    break;
                  }
                }
                if(dst_window != 0)
                {
                  Temp scratch = scratch_begin(0, 0);
                  String8 cmd_spec_string = df_cmd_name_part_from_string(cmd_string);
                  DF_CmdSpec *cmd_spec = df_cmd_spec_from_string(cmd_spec_string);
                  if(!df_cmd_spec_is_nil(cmd_spec))
                  {
                    DF_CmdParams params = df_cmd_params_from_gfx();
                    DF_CtrlCtx ctrl_ctx = df_ctrl_ctx_from_window(dst_window);
                    String8 error = df_cmd_params_apply_spec_query(scratch.arena, &ctrl_ctx, &params, cmd_spec, df_cmd_arg_part_from_string(cmd_string));
                    if(error.size == 0)
                    {
                      df_push_cmd__root(&params, cmd_spec);
                    }
                    else
                    {
                      DF_CmdParams params = df_cmd_params_from_window(dst_window);
                      params.string = error;
                      df_cmd_params_mark_slot(&params, DF_CmdParamSlot_String);
                      df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Error));
                    }
                  }
                  scratch_end(scratch);
                }
              }
              os_semaphore_drop(ipc_semaphore);
            }
          }
          
          //- rjf: update & render frame
          OS_Handle repaint_window = {0};
          update_and_render(repaint_window, 0);
          
          //- rjf: auto run
          if(auto_run)
          {
            auto_run = 0;
            DF_CmdParams params = df_cmd_params_from_gfx();
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_LaunchAndRun));
          }
          
          //- rjf: auto step
          if(auto_step)
          {
            auto_step = 0;
            DF_CmdParams params = df_cmd_params_from_gfx();
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_StepInto));
          }
          
          //- rjf: jit attach
          if(jit_attach)
          {
            jit_attach = 0;
            DF_CmdParams params = df_cmd_params_from_gfx();
            df_cmd_params_mark_slot(&params, DF_CmdParamSlot_ID);
            params.id = jit_pid;
            df_push_cmd__root(&params, df_cmd_spec_from_core_cmd_kind(DF_CoreCmdKind_Attach));
          }
          
          //- rjf: quit if no windows are left
          if(df_gfx_state->first_window == 0)
          {
            break;
          }
        }
      }
      
    }break;
    
    //- rjf: inter-process communication message sender
    case ExecMode_IPCSender:
    {
      Temp scratch = scratch_begin(0, 0);
      
      //- rjf: grab ipc shared memory
      OS_Handle ipc_shared_memory = os_shared_memory_open(ipc_shared_memory_name);
      void *ipc_shared_memory_base = os_shared_memory_view_open(ipc_shared_memory, r1u64(0, MB(16)));
      if(ipc_shared_memory_base != 0)
      {
        OS_Handle ipc_semaphore = os_semaphore_open(ipc_semaphore_name);
        IPCInfo *ipc_info = (IPCInfo *)ipc_shared_memory_base;
        if(os_semaphore_take(ipc_semaphore, os_now_microseconds() + Million(6)))
        {
          U8 *buffer = (U8 *)(ipc_info+1);
          U64 buffer_max = IPC_SHARED_MEMORY_BUFFER_SIZE - sizeof(IPCInfo);
          StringJoin join = {str8_lit(""), str8_lit(" "), str8_lit("")};
          String8 msg = str8_list_join(scratch.arena, &cmdln.inputs, &join);
          ipc_info->msg_size = Min(buffer_max, msg.size);
          MemoryCopy(buffer, msg.str, ipc_info->msg_size);
          os_semaphore_drop(ipc_semaphore);
        }
      }
      
      scratch_end(scratch);
    }break;
    
    //- rjf: built-in pdb/dwarf -> raddbg converter mode
    case ExecMode_Converter:
    {
      Temp scratch = scratch_begin(0, 0);
      
      //- rjf: parse arguments
      PDBCONV_Params *params = pdb_convert_params_from_cmd_line(scratch.arena, &cmdln);
      
      //- rjf: open output file
      String8 output_name = push_str8_copy(scratch.arena, params->output_name);
      OS_Handle out_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_Write, output_name);
      B32 out_file_is_good = !os_handle_match(out_file, os_handle_zero());
      
      //- rjf: convert
      PDBCONV_Out *out = 0;
      if(out_file_is_good)
      {
        out = pdbconv_convert(scratch.arena, params);
      }
      
      //- rjf: bake file
      if(out != 0 && params->output_name.size > 0)
      {
        String8List baked = {0};
        cons_bake_file(scratch.arena, out->root, &baked);
        U64 off = 0;
        for(String8Node *node = baked.first; node != 0; node = node->next)
        {
          os_file_write(out_file, r1u64(off, off+node->string.size), node->string.str);
          off += node->string.size;
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
                                    "--profile:<path>\n"
                                    "Use to specify the location of a profile file which should be used. Profile files are used to store settings for users and projects. If this file does not exist, it will be created as necessary. This file will be autosaved as profile-related changes are made.\n\n"
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

////////////////////////////////
//~ rjf: Low-Level Entry Points

//- rjf: windows
#if OS_WINDOWS

global DWORD g_saved_exception_code = 0;

internal DWORD
win32_exception_filter(DWORD dwExceptionCode)
{
  g_saved_exception_code = dwExceptionCode;
  return EXCEPTION_EXECUTE_HANDLER;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
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
  static TCTX main_thread_tctx = {0};
  tctx_init_and_equip(&main_thread_tctx);
  Arena *perm_arena = arena_alloc();
  WCHAR *command_line = GetCommandLineW();
  int argc;
  WCHAR **argv_16 = CommandLineToArgvW(command_line, &argc);
  char **argv = push_array(perm_arena, char *, argc);
  for(int i = 0; i < argc; i += 1)
  {
    String16 arg16 = str16_cstring((U16 *)argv_16[i]);
    String8 arg8 = str8_from_16(perm_arena, arg16);
    argv[i] = (char *)arg8.str;
  }
  __try
  {
    entry_point(argc, argv);
  }
  __except(win32_exception_filter(GetExceptionCode()))
  {
    char buffer[256] = {0};
    raddbg_snprintf(buffer, sizeof(buffer), "A fatal exception (code 0x%x) occurred. The process is terminating.", (U32)g_saved_exception_code);
    os_graphical_message(1, str8_lit("Fatal Exception"), str8_cstring(buffer));
    ExitProcess(1);
  }
  return 0;
}

//- rjf: linux
#elif OS_LINUX

int main(int argument_count, char **arguments)
{
  static TCTX main_thread_tctx = {0};
  tctx_init_and_equip(&main_thread_tctx);
  entry_point(argument_count, arguments);
  return 0;
}

#endif
