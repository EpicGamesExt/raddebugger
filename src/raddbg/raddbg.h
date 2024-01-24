// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Frontend/UI Pass Tasks
//
// [ ] source view -> floating margin/line-nums
// [ ] theme colors -> more explicit about e.g. opaque backgrounds vs. floating
//     & scrollbars etc.
// [ ] drag/drop tab cleanup
// [ ] target/breakpoint/watch-pin reordering
// [ ] watch window reordering
// [x] query views, cleanup & floating - maybe merge "applies to view" vs. not
// [ ] standard way to filter
// [ ] visualize remapped files (via path map)
// [ ] hovering truncated string for a short time -> tooltip with full wrapped
//     string
// [ ] theme lister -> fonts & font sizes
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
// [ ] autocomplete lister should respect position in edited expression,
//     tabbing through should autocomplete but not exit, etc.
//
//  [ ]  it would be nice to have "show in explorer" for right click on source
//       file tab (opens explorer & selects the file)
//  [x]  it would be nice if Alt+o in source file would switch between .h and
//       .c/cpp file (just look for same name in same folder)
//
//  [ ]  what's up with decimal number coloring where every group of 3 are in
//       different color? can I turn it off? And why sometimes digits in number
//       start with brighter color, but sometimes with darker - shouldn't it
//       always have the same color ordering?
//
//  [ ]  middle mouse button on tab should close it

////////////////////////////////
//~ rjf: Hot, High Priority Tasks (Complete Unusability, Crashes, Fire-Worthy)
//
// [ ] ** Thread/process control bullet-proofing, including solo-step mode
//
// [ ] ** In solo-stepping mode, if I step over something like CreateFileA, it
//     pseudo-hangs the debugger. I can't seem to do anything else, including
//     "Kill All". I have to close the debugger and restart it, AFAICT?
//
// [ ] ** I tried to debug a console program, and "step into" didn't seem to
//     work. Instead, it just started running the program, but the program
//     seemed to hang, and then the debugger pseudo-hung with a continual
//     progress bar in the disassembly window. I had to close and restart. Is
//     console app debugging not working yet, perhaps?
//
// [ ] Setting the code_font/main_font values to a font name doesn't work.
//     Should probably make note that you have to set it to a path to a TTF,
//     since that's not normally how Windows fonts work.
//
// [ ] ** Converter performance & heuristics for asynchronously doing it early
//
// [ ] disasm animation & go-to-address
//
// [ ] visualize remapped files (via path map)

////////////////////////////////
//~ rjf: Hot, Medium Priority Tasks (Low-Hanging-Fruit Features, UI Jank, Cleanup)
//
// [ ] Watch Window Type Evaluation
// [ ] Globals, Thread-Locals, Types Views
// [ ] Watch Window Filtering
//
// [ ] investigate /DEBUG:FASTLINK - can we somehow alert that we do not
//     support it?
//
// [ ] escaping in config files - breakpoint labels etc.
// [ ] focus changing between query bar & panel content via mouse
//
// [ ] ** while typing, "Alt" Windows menu things should not happen
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
//
// [ ] ** "Find Name" may not be working as advertised. In the description, it
//     says you can jump to a file, but if I type in the complete filename of
//     a file in the project and hit return, it just turns red and says it
//     couldn't find it. This happens even if the file is already open in a
//     tab.
//   [ ] "Find Name" would be a lot more useful if you could type partial
//       things, and it displayed a list, more like what happens in a
//       traditional text editor. Typing the entire name of a function to jump
//       to it is too laborious.
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
//  [ ] ** Function breakpoints should show up in the source listing. Without
//      them being visible, it is confusing when you run and you stop there,
//      because you're like "wait why did it stop" and then you later remember
//      that's because there was a function breakpoint there.
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
//  [ ] Using the word "symbol" in "Code (Symbol)" seems like a bad idea, since
//      you're referring to non-identifier characters, but in a debugger
//      "symbol" usually means something defined in the debug information.
//
//  [ ] I LOVE ALT-W to add watch under cursor, but I would prefer to have it
//      add what's under the MOUSE cursor instead of the keyboard cursor. Can
//      we get a command for that so I can bind ALT-W to that instead?
//
//  [ ] For theme editing, when you hove the mouse over a theme color entry and
//      it highlights that entry, it might help to temporarily change that
//      color to white (or the inverse of the background color, or whatever) so
//      that the user can see what things on the screen use that theme color.
//
//  [ ] I couldn't figure out how to affect the "dim" color in constants that
//      have alternating bright/dim letters to show sections of a number. Is
//      this in the theme colors somewhere?
//
//  [ ] For breakpoint-on-function, it would be great if it showed a list of
//      (partial) matches as you type, so you can stop typing once it gets the
//      right function instead of having to type the entire function name.
//
//  [ ] Hovering over a source tab that is clipped should probably display the
//      full thing that was in that tab (like the whole filename, etc.). Right
//      now, hovering does nothing AFAICT.
//
//  [ ] ** I couldn't figure out how to really view threads in the debugger.
//      The only place I found a thread list was in "The Scheduler", but it
//      only lists threads by ID, which is hard to use. I can hover over them
//      to get the stack, which helps, but it would be much nicer if the top
//      function was displayed in the window by default next to the thread.
//  [ ] ** It would be nice if thread listings displayed the name of the
//      thread, instead of just the ID.
//
//  [ ] ** Scrollbars are barely visible for me, for some reason. I could not
//      find anything in the theme that would fill them with a solid, bright
//      color. Instead they are just a thin outline and the same color as the
//      scroll bar background.
//
//  [ ] Dragging a window tab (like Locals or Registers or whatnot) and
//      canceling with ESC should revert the window tab to where it was.
//      Currently, it leaves the window tab reordered if you dragged over its
//      window and shuffled its position.
//
//  [ ] Many of the UI elements, like the menus, would like better if they had
//      a little bit of margin. Having the text right next to the edges, and
//      with no line spacing, makes it harder to read things quickly.
//
//  [ ] Menus take too long to show up. I would prefer it if they were instant.
//      The animation doesn't really provide any useful cues, since I know
//      where the menu came from.
//
//  [ ] Theme window should include font scaling. I was able to find the
//      command for increasing the font scale, but I imagine most people
//      wouldn't think to look there.
//  [ ] I had to go into the user file to change the font. That should probably
//      be in the theme window?
//
//  [ ] The way the "commands" view worked was idiosyncratic. All the other
//      views stay up, but that one goes away whenever I select a command for
//      some reason.
//   [ ] Also, I could not move the commands window anywhere AFAICT. It seems
//       to just pop up over whatever window I currently have selected. This
//       would make sense for a hotkey (which I assume is the way it was
//       designed), but it seems like it should be permanent if you can select
//       it from the View menu.
//  [ ] If the command window is not wide enough, you cannot read the
//      description of a command because it doesn't word-wrap, nor can you
//      hover over it to get the description in a tooltip (AFAICT).
//
//  [ ] It'd be nice to have a "goto byte" option for source views, for jumping
//      to error messages that are byte-based instead of line-based.
//
//  [ ] Pressing the left mouse button on the menu bar and dragging does not
//      move through the menus as expected - instead, it opens the one you
//      clicked down on, then does nothing until you release, at which point it
//      opens the menu you released on.
//  [ ] Similarly, pressing the left mouse button on a menu and dragging to an
//      item, then releasing, does not trigger that item as expected. Instead,
//      it is a nop, and it waits for you to click again on the item.
//
//  [ ] Working with panels felt cumbersome. I couldn't figure out any way to
//      quickly arrange the display without manually selecting "split panel"
//      and "close panel" and stuff from the menu, which took a long time.
//   - @polish @feature ui for dragging tab -> bundling panel split options
//
//  [ ] I found the "context menu" convention to be confusing. For example, if
//      I left-click on a tab, it selects the tab. If I right-click on a tab,
//      it opens the context menu. However, if I left-click on a module, it
//      opens the context window. It seems like maybe menus should be right,
//      and left should do the default action, more consistently?
//
//  [ ] Hovering over disassembly highlights blocks of instructions, which I
//      assume correspond to source lines. But perhaps it should also highlight
//      the source lines? The inverse hover works (you hover over source, and
//      it highlights ASM), but ASM->source doesn't.
//
//  [ ] It wasn't clear to me how you save a user or profile file. I can see
//      how to load them, but not how you save them. Obviously I can just copy
//      the files myself in the shell, but it seemed weird that there was no
//      "save" option in the menus.
//
// [ ] @cleanup @feature double & triple click select in source views
// [ ] @feature hovering truncated text in UI for some amount of time -> show
//     tooltip with full text
// [ ] @feature disasm keyboard navigation & copy/paste
// [ ] @feature debug info overrides (both path-based AND module-based)
// [ ] configure tab size
// [ ] run-to-line needs to work if no processes are running
//     - place temp bp, attach "die on hit" flag or something like that?
// [ ] auto-scroll output window
//
// [ ] C++ single & multi inheritance member visualization in watch window

////////////////////////////////
//~ rjf: Hot, Low Priority Tasks (UI Opinions, Less-Serious Jank, Preferences, Cleanup)
//
//  [ ] ** Directory picking is kind of busted, as it goes through the same
//      path as file picking, and this doesn't give the user a clean path to
//      actually pick a folder, just navigate with them
//
//  [ ] ** In the call stack, I would like to be able to click quickly and move
//      around the stack. Right now, you can do that with the first and third
//      column, but the second column drops down a context menu. Since right
//      click is already for context menus, can it not just be that double-
//      clicking any column jumps to that stack frame?
//
//  [ ] ** I find it really hard to read the code with the heavyweight lines
//      running through it for breakpoints and stepping and things. Is there a
//      way to turn the lines off? AFAICT they are based on thread and
//      breakpoint color, so you can't really control the line drawing? I might
//      be fine with them, but they would have to be much more light (like
//      alpha 0.1 or something)
//
//  [ ] It's confusing that ENTER is the way you expand and collapse things in
//      the watch window, but then also how you edit them if they are not
//      expandable? It seems like this should be consistent (one way to edit,
//      one way to expand/collapse, that are distinct)
//
//  [ ] I didn't understand the terminology "Equip With Color". Does that just
//      mean specify the color used to display it? Is "Apply Color" perhaps a
//      bit more user-friendly?
//
//  [ ] The cursor feels a bit too huge vertically.
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
//  [ ] Hitting ESC during a color picker drag should abort the color picking
//      and revert to the previous color. Currently, it just accepts the last
//      drag result as the new color.
//
//  [ ] It was not clear to me why a small "tab picker" appeared when I got to
//      a certain number of tabs. It seemed to appear even if the tabs were
//      quite large, and there was no need to a drop-down menu to pick them. It
//      feels like either it should always be there, or it should only show up
//      if at least one tab gets small enough to have its name cut off?
//
//  [ ]  can it ignore stepping into _RTC_CheckStackVars generated functions?
//  [ ]  mouse back button should make view to go back after I double clicked
//       on function to open it
//  [ ]  pressing random keyboard keys in source code advances text cursor like
//       you were inputting text, very strange.
//  [ ]  Alt+8 to switch to disassembly would be nice (regardless on which
//       panel was previous, don't want to use ctrl+, multiple times)
//       Alt+8 for disasm and Alt+6 for memory view are shortcuts I often use
//       in VS
//  [ ]  in watch window when I enter some new expression and then click mouse
//       away from cell, then it should behave the same as if I pressed enter.
//       Currently it does the same as if I have pressed esc and I have lost my
//       expression
//  [ ]  default font size is too small for me - not only source code, but
//       menus/tab/watch names (which don't resize). Maybe you could query
//       Windows for initial font size?
//  [ ]  zooming behaves very strangely - sometimes it zooms source code,
//       sometimes both source code and menu/tab/watch font size, sometimes
//       just menu/tab/watch font size not source size.
//  [ ]  icon fonts glyphs sometimes disappear for specific font size, but they
//       reappear if you go +1 higher or -1 lower. Mostly red triangle in watch
//       values for "unknown identifier". But also yellow arrow in call stack
//       disappears if font size gets too large.
//  [ ]  undo close tab would be nice. If not for everything, then at least
//       just for source files

////////////////////////////////
//~ rjf: Hot, Feature Tasks (Not really "low priority" but less urgent than fixes)
//
// [ ] Fancy View Rules
//  [ ] table column boundaries should be checked against *AFTER* table
//      contents, not before
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
// [ ] @feature entity views: filtering & reordering

////////////////////////////////
//~ rjf: Cold, Clean-up Tasks That Probably Only Ryan Notices
// (E.G. Because They Are Code-Related Or Because Nobody Cares)
//
// [ ] @bug view-snapping in scroll-lists, accounting for mapping between
//     visual positions & logical positions (variably sized rows in watch,
//     table headers, etc.)
// [ ] @bug selected frame should be keyed by run_idx or something so that it
//     can gracefully reset to the top frame when running
// [ ] @cleanup collapse DF_CfgNodes into just being MD trees, find another way
//     to encode config source - don't need it at every node
// [ ] @cleanup straighten out index/number space & types & terminology for
//     scroll lists
// [ ] @cleanup simplification pass over eval visualization pipeline & types,
//     including view rule hooks
// [ ] @cleanup naming pass over eval visualization part of the frontend,
//     "blocks" vs. "canvas" vs. "expansion" - etc.
// [ ] @cleanup central worker thread pool - eliminate per-layer thread pools

////////////////////////////////
//~ rjf: Cold, Unsorted Notes (Deferred Until Existing Lists Mostly Exhausted)
//
// [ ] @feature types -> auto view rules (don't statefully fill view rules
//     given types, just query if no other view rule is present, & autofill
//     when editing)
// [ ] @feature eval system -> somehow evaluate breakpoint hit counts? "meta"
//     variables?
// [ ] @feature watch window labels
// [ ] @feature scheduler -> thread grid view?
//
// [ ] @feature disasm view improvement features
//  [ ] interleaved src/dasm view
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
// [ ] ui code maintenance, simplification, design, & robustness pass
//  [ ] page-up & page-down correct handling in keyboard nav
//  [ ] collapse context menus & command lister into same codepaths. filter by
//      context. parameterize by context.
//  [ ] collapse text cells & command lister & etc. into same codepath (?)
//  [ ] nested context menus
//  [ ] unified top-level cursor/typing/lister helper
//  [ ] font selection lister
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

#ifndef RADDBG_H
#define RADDBG_H

////////////////////////////////
//~ rjf: Build Settings

#define RADDBG_VERSION_MAJOR 0
#define RADDBG_VERSION_MINOR 9
#define RADDBG_VERSION_PATCH 6
#define RADDBG_VERSION_STRING_LITERAL Stringify(RADDBG_VERSION_MAJOR) "." Stringify(RADDBG_VERSION_MINOR) "." Stringify(RADDBG_VERSION_PATCH)
#if defined(NDEBUG)
# define RADDBG_TITLE_STRING_LITERAL "The RAD Debugger (" RADDBG_VERSION_STRING_LITERAL " ALPHA) - " __DATE__ ""
#else
# define RADDBG_TITLE_STRING_LITERAL "The RAD Debugger (" RADDBG_VERSION_STRING_LITERAL " ALPHA) - " __DATE__ " [Debug]"
#endif
#define RADDBG_GITHUB_ISSUES "https://github.com/EpicGames/raddebugger/issues"

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

#define IPC_SHARED_MEMORY_BUFFER_SIZE MB(16)
StaticAssert(IPC_SHARED_MEMORY_BUFFER_SIZE > sizeof(IPCInfo), ipc_buffer_size_requirement);
read_only global String8 ipc_shared_memory_name = str8_lit_comp("_raddbg_ipc_shared_memory_");
read_only global String8 ipc_semaphore_name = str8_lit_comp("_raddbg_ipc_semaphore_");
global U64 frame_time_us_history[64] = {0};
global U64 frame_time_us_history_idx = 0;
global Arena *leftover_events_arena = 0;
global OS_EventList leftover_events = {0};

////////////////////////////////
//~ rjf: Frontend Entry Points

internal void update_and_render(OS_Handle repaint_window_handle, void *user_data);

#endif // RADDBG_H
