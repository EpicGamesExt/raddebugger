// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: 0.9.16 release notes
//
// - Auto view rules have been upgraded to support type pattern-matching. This
//   makes them usable for generic types in various languages. This
//   pattern-matching is done by using `?` characters in a pattern, to specify
//   placeholders for various parts of a type name. For example, the pattern
//   `DynamicArray<?>` would match `DynamicArray<int>`, `DynamicArray<float>`,
//   and so on.
// - The `slice` view rule has been streamlined and simplified. When an
//   expression with a `slice` view rule is expanded, it will simply expand to
//   the slice's contents, which matches the behavior with static arrays.
// - The `slice` view rule now supports `first, one_past_last` pointer pairs,
//   as well as `base_pointer, length` pairs.
// - The syntax for view rules has changed to improve its familiarity,
//   usability, and to improve its consistency with the rest of the evaluation
//   language. It now appears like function calls, and many arguments no longer
//   need to be explicitly named. For example, instead of
//   `bitmap:{w:1024, h:1024}`, the new view rule syntax would be
//   `bitmap(1024, 1024)`. Commas can still be omitted. Named arguments are
//   still possible (and required in some cases):
//   `disasm(size=1024, arch=x64)`. If there are no arguments required for a
//   view rule, like `hex`, then no parentheses are needed.
// - The hover evaluation feature has been majorly upgraded to support all
//   features normally available in the `Watch` view. Hover evaluations now
//   explicitly show type information, and contain a view rule column, which
//   can be edited in a way identical to the `Watch` view.
// - Added "auto tabs", which are colored differently than normal tabs. These
//   tabs are automatically opened by the debugger when snapping to source code
//   which is not already opened. They are automatically replaced and recycled
//   when the debugger needs to snap to new locations. This will greatly reduce
//   the number of unused source code tabs accumulated throughout a debugging
//   session. These tabs can still be promoted to permanent tabs, in which case
//   they'll stay around until manually closed.
// - The `Scheduler` view has been removed, in favor of three separate views,
//   which present various versions of the same information: `Threads`,
//   `Processes`, and `Machines`. The justification for this is that the
//   common case is debugging a small number of programs, usually 1, and for
//   those purposes, the `Threads` view is sufficient if not desirable, and
//   the extra information provided by `Processes` and `Machines`, while useful
//   in other contexts, is not useful in that common case. The `Machines` view
//   now shows a superset of the information previously found in the
//   `Scheduler` view.
// - The two separate interfaces for editing threads, breakpoints, and watch
//   pins (the right-click context menu and the dedicated tabs) have been
//   merged. Both interfaces support exactly the same features in exactly the
//   same way.
// - Added the ability to add a list of environment strings to targets.
// - The debugger releases are now packaged with a `raddbg_markup.h`
//   single-header library which contains a number of source code markup tools
//   which can be used in your programs. Some of these features are for direct
//   interaction with the RAD Debugger. Others are simply helpers (often
//   abstracting over platform functionality) for very commonly needed
//   debugger-related features, like setting thread names. This second set of
//   features will work with any debugger. Here is a list of this library's
//   initial features (proper documentation will be written once the library
//   matures):
//   - `raddbg_is_attached()`: Returns 1 if the RAD Debugger is attached,
//     otherwise returns 0.
//   - `raddbg_break()`: Generates a trap instruction at the usage site.
//   - `raddbg_break_if(expr)`: Generates a trap instruction guarded by a
//     conditional, evaluating `expr`.
//   - `raddbg_thread_name(format, ...)`, e.g.
//     `raddbg_thread_name("worker_thread_%i", index)`: Sets the calling
//     thread's name.
//   - `raddbg_log(format, ...)`, e.g.
//     `raddbg_log("This is a number: %i", 123)`: Writes a debug string for a
//     debugger to read and display in its UI.
//   - `raddbg_thread_color_hex(hexcode)`, e.g.
//     `raddbg_thread_color_hex(0xff0000ff)`: Sets the calling thread's color.
//     - Also can be done with individual `[0, 1]` color components:
//       `raddbg_thread_color_rgba(1.f, 0.f, 0.f, 1.f)`
//   - `raddbg_pin(<expr>, <view_rule>)`, e.g.
//     `raddbg_pin(dynamic_array, slice)`: Like watch pins, but defined in source
//     code. This is used by the debugger UI, but does not show up anywhere
//     other than the source code, so it can either be used as a macro (which
//     expands to nothing), or in comments too.
//   - `raddbg_entry_point(name)`, e.g. `raddbg_entry_point(entry_point)`:
//     declares the entry point for an executable, which the debugger will use
//     when stepping into a program, rather than the defaults (e.g. `main`).
//   - `raddbg_auto_view_rule(<type|pattern>, <view_rule>)`, e.g.
//     `raddbg_auto_view_rule(DynamicArray<?>, slice)`: declares an
//     auto-view-rule from source code, rather than from debugger
//     configuration.
// - Fixed an annoyance where the debugger would open a console window, even
//   for graphical programs, causing a flicker.
// - Made several visual improvements.

////////////////////////////////
//~ rjf: feature cleanup, code dedup, code elimination pass:
//
// [ ] 'view rules' need to be rephrased as "function" calls in the expression language
// [ ] need a formalization which takes unknown identifiers which are called, and tries
//     to use that to apply a IR-generation rule, which is keyed by that unknown
//     identifier
// [ ] we need to select expressions as "parents" when possible, so that when using an
//     auto-view-rule (or similar context), leaf identifiers referring to e.g. members
//     of an expression's type resolve correctly (e.g. bitmap(base, width, height) being
//     used as a shorthand for bitmap(foo.base, foo.width, foo.height) when evaluating
//     foo).
// [ ] *ALL* expressions in watch windows need to be editable.
//
// [ ] config hot-reloading, using cfg wins
// [ ] undo/redo, using cfg wins
// [ ] back/forward, using cfg wins
// [ ] autocompletion lister, file lister, function lister, command lister,
//     etc., all need to be merged, and optionally contextualized/filtered.
//     right-clicking a tab should be equivalent to spawning a command lister,
//     but only with commands that are directly
//
// [ ] r8 bitmap view rule seems incorrect?
// [ ] crash bug, release mode - filter globals view (try with debugging raddbg, typing `dev` in globals view)
//
// [ ] stepping-onto a line with a conditional breakpoint, which fails, causes a
// single step over the first instruction of that line, even if the thread
// would've stopped at the first instruction due to the step, were that bp not
// there.
//
// [ ] if a breakpoint matches the entry point's starting address, its hit count
// is not correctly incremented.

////////////////////////////////
//~ rjf: post-0.9.12 TODO notes
//
// [ ] breakpoints in optimized code? maybe early-terminating bp resolution loop? @bpmiss
//      - actually this seems to be potentially because of incomplete src-line-map info...
// [ ] Mohit-reported breakpoint not hitting - may be similar thing to @bpmiss
//
// [ ] CLI argument over-mangling?
// [ ] fix light themes
// [ ] make `array` view rule work with actual array types, to change their
//     size dynamically
// [ ] single-line visualization busted with auto-view-rules applied, it seems...
//     not showing member variables, just commas, check w/ mohit
// [ ] disasm starting address - need to use debug info for more correct
//     results...
//  [ ] linked list view rule
//  [ ] output: add option for scroll-to-bottom - ensure this shows up in universal ctx menu
//  [ ] EVAL LOOKUP RULES -> currently going 0 -> rdis_count, but we need
//  to prioritize the primary rdi
//  [ ] (reported by forrest) 'set-next-statement' -> prioritize current
//      module/symbol, in cases where one line maps to many voffs
//  [ ] universal ctx menu address/watch options; e.g. watch -> memory; watch -> add watch
//  [ ] rich hover coverage; bitmap <-> geo <-> memory <-> disassembly <-> text; etc.
//  [ ] visualize all breakpoints everywhere - source view should show up in
//      disasm, disasm should show up in source view, function should show up in
//      both, etc.
//    [ ] ** Function breakpoints should show up in the source listing. Without
//        them being visible, it is confusing when you run and you stop there,
//        because you're like "wait why did it stop" and then you later remember
//        that's because there was a function breakpoint there.

////////////////////////////////
//~ rjf: Frontend/UI Pass Tasks
//
// [ ] theme lister -> fonts & font sizes
// [ ] "Browse..." buttons should adopt a more relevant starting search path,
//     if possible
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
// [ ] use backslashes on windows by default, forward slashes elsewhere (?)
//
// [ ] investigate /DEBUG:FASTLINK - can we somehow alert that we do not
//     support it?
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
//  [ ] ** One very nice feature of RemedyBG that I use all the time is the
//      ability to put "$err, hr" into the watch window, which will just show
//      the value of GetLastError() as a string. This is super useful for
//      debugging, so you don't have to litter your own code with it.
//      (NOTE(rjf): NtQueryInformationThread)
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
// [ ] Jump table thunks, on code w/o /INCREMENTAL:NO

////////////////////////////////
//~ rjf: Hot, Feature Tasks (Not really "low priority" but less urgent than fixes)
//
// [ ] eval wide/async transforms (e.g. diff(blob1, blob2))
// [ ] search-in-all-files
// [ ] Memory View
//  [ ] memory view mutation controls
//  [ ] memory view user-made annotations
// [ ] globally disable/configure default view rule-like things (string
//     viz for u8s in particular)
// [ ] @feature processor/data breakpoints
// [ ] @feature automatically snap to search matches when searching source files
// [ ] automatically start search query with selected text

////////////////////////////////
//~ rjf: Cold, Clean-up Tasks That Probably Only Ryan Notices
// (E.G. Because They Are Code-Related Or Because Nobody Cares)
//
// [ ] @bug view-snapping in scroll-lists, accounting for mapping between
//     visual positions & logical positions (variably sized rows in watch,
//     table headers, etc.)
// [ ] @cleanup straighten out index/number space & types & terminology for
//     scroll lists
// [ ] @cleanup eliminate explicit font parameters in the various ui paths (e.g.
//     code slice params)

////////////////////////////////
//~ rjf: Cold, Unsorted Notes (Deferred Until Existing Lists Mostly Exhausted)
//
// [ ] @feature disasm view improvement features
//  [ ] visualize jump destinations in disasm
//
// [ ] @feature eval ui improvement features
//  [ ] serializing eval view maps
//  [ ] view rule hook coverage
//  [ ] `array:(x, y)` - multidimensional array
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
//  [ ] font cache layer -> can probably cache (string*font*size) -> (run) too
//      (not just rasterization)... would save a *lot*, there is a ton of work
//      just in looking up & stitching stuff repeatedly
//  [ ] convert UI layout pass to not be naive recursive version
//  [ ] (big change) parallelize window ui build codepaths per-panel

////////////////////////////////
//~ rjf: Recently Completed Task Log
//
// [x] filesystem drag/drop support
// [x] ** Converter performance & heuristics for asynchronously doing it early
//  [x]  icon fonts glyphs sometimes disappear for specific font size, but they
//       reappear if you go +1 higher or -1 lower. Mostly red triangle in watch
//       values for "unknown identifier". But also yellow arrow in call stack
//       disappears if font size gets too large.
// [x] @cleanup central worker thread pool - eliminate per-layer thread pools
// [x] frontend config entities, serialization/deserialization, remove hacks,
//     etc. - the entity structure should be dramatically simplified & made
//     to reflect a more flexible string-tree data structure which can be
//     more trivially derived from config, and more flexibly rearranged.
//     drag/drop watch rows -> tabs, tabs -> watch rows, etc.
// [x] frontend entities need to be the "upstream state" for windows, panels,
//     tabs, etc. - entities can be mapped to caches of window/panel/view state
//     in purely immediate-mode fashion, so the only *state* part of the
//     equation only has to do with the string tree.
// [x] watch table UI - hidden table boundaries, special-cased control hacks
// [x] hash store -> need to somehow hold on to hash blobs which are still
//     depended upon by usage layers, e.g. extra dependency refcount, e.g.
//     text cache can explicitly correllate nodes in its cache to hashes,
//     bump their refcount - this would keep the hash correllated to its key
//     and it would prevent it from being evicted (output debug string perf)
// [x] OutputDebugString spam, keeping way too much around!
// [x] auto view rule templates (?)
// [x] auto-view-rules likely should apply at each level in the expression
//     tree
// [x] `slice` view rule - extend to support begin/end style as well
//  [x] investigate false exceptions, being reported while stepping through init code
//  [x] collapse upstream state for theme/bindings/settings into entities; use cache accelerators if needed to make up difference
//  [x] collapse upstream state for windows/panels/tabs into entities; use downstream window/view resource cache to make up the difference
//  [x] entity <-> mdesk paths
//  [x] save view column pcts; generalize to being a first-class thing in
//      RD_View, e.g. by just having a string -> f32 store
// [x] transient view timeout releasing
//  [x] view rule editors in hover-eval
//  [x] table column boundaries should be checked against *AFTER* table
//      contents, not before
//  [x] In a "hover watch" (where you hover over a variable and it shows a pop-
//      up watch window), if you expand an item near the bottom of the listing,
//      it will be clipped to the bottom of the listing instead of showing the
//      actual items (ie., it doesn't resize the listing based on what's
//      actually visible)
// [x] Right-clicking on a thread in the Scheduler window pops up a context
//     menu, but you can't actually see it because the tooltip for the thread
//     draws on top of it, so you can't see the menu.
// [x] double-click vs. single-click for folder navigation, see if we can infer

////////////////////////////////
//~ rjf: Build Options

#define BUILD_TITLE "The RAD Debugger"
#define OS_FEATURE_GRAPHICAL 1

#define R_INIT_MANUAL 1
#define TEX_INIT_MANUAL 1
#define GEO_INIT_MANUAL 1
#define FNT_INIT_MANUAL 1
#define D_INIT_MANUAL 1
#define RD_INIT_MANUAL 1
#define P2R_INIT_MANUAL 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [lib]
#include "third_party/rad_lzb_simple/rad_lzb_simple.h"
#include "third_party/rad_lzb_simple/rad_lzb_simple.c"

//- rjf: [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "async/async.h"
#include "rdi_format/rdi_format_local.h"
#include "rdi_make/rdi_make_local.h"
#include "mdesk/mdesk.h"
#include "hash_store/hash_store.h"
#include "file_stream/file_stream.h"
#include "text_cache/text_cache.h"
#include "mutable_text/mutable_text.h"
#include "path/path.h"
#include "coff/coff.h"
#include "coff/coff_parse.h"
#include "pe/pe.h"
#include "codeview/codeview.h"
#include "codeview/codeview_parse.h"
#include "msf/msf.h"
#include "msf/msf_parse.h"
#include "pdb/pdb.h"
#include "pdb/pdb_parse.h"
#include "pdb/pdb_stringize.h"
#include "rdi_from_pdb/rdi_from_pdb.h"
#include "regs/regs.h"
#include "regs/rdi/regs_rdi.h"
#include "dbgi/dbgi.h"
#include "dasm_cache/dasm_cache.h"
#include "demon/demon_inc.h"
#include "eval/eval_inc.h"
#include "eval_visualization/eval_visualization_inc.h"
#include "ctrl/ctrl_inc.h"
#include "font_provider/font_provider_inc.h"
#include "render/render_inc.h"
#include "ptr_graph_cache/ptr_graph_cache.h"
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
#include "async/async.c"
#include "rdi_format/rdi_format_local.c"
#include "rdi_make/rdi_make_local.c"
#include "mdesk/mdesk.c"
#include "hash_store/hash_store.c"
#include "file_stream/file_stream.c"
#include "text_cache/text_cache.c"
#include "mutable_text/mutable_text.c"
#include "path/path.c"
#include "coff/coff.c"
#include "coff/coff_parse.c"
#include "pe/pe.c"
#include "codeview/codeview.c"
#include "codeview/codeview_parse.c"
#include "msf/msf.c"
#include "msf/msf_parse.c"
#include "pdb/pdb.c"
#include "pdb/pdb_parse.c"
#include "pdb/pdb_stringize.c"
#include "rdi_from_pdb/rdi_from_pdb.c"
#include "regs/regs.c"
#include "regs/rdi/regs_rdi.c"
#include "dbgi/dbgi.c"
#include "dasm_cache/dasm_cache.c"
#include "demon/demon_inc.c"
#include "eval/eval_inc.c"
#include "eval_visualization/eval_visualization_inc.c"
#include "ctrl/ctrl_inc.c"
#include "font_provider/font_provider_inc.c"
#include "render/render_inc.c"
#include "ptr_graph_cache/ptr_graph_cache.c"
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
  ThreadNameF("[rd] ipc signaler thread");
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
          
          //- rjf: unpack command line inputs
          String8 executable_name_string = {0};
          String8 arguments_string = {0};
          String8 working_directory_string = {0};
          {
            // rjf: unpack full executable path
            if(args.first->string.size != 0)
            {
              String8 current_path = os_get_current_path(scratch.arena);
              String8 exe_name = args.first->string;
              PathStyle style = path_style_from_str8(exe_name);
              if(style == PathStyle_Relative)
              {
                exe_name = push_str8f(scratch.arena, "%S/%S", current_path, exe_name);
                exe_name = path_normalized_from_string(scratch.arena, exe_name);
              }
              executable_name_string = exe_name;
            }
            
            // rjf: unpack working directory
            if(args.first->string.size != 0)
            {
              String8 path_part_of_arg = str8_chop_last_slash(args.first->string);
              if(path_part_of_arg.size != 0)
              {
                String8 path = push_str8f(scratch.arena, "%S/", path_part_of_arg);
                working_directory_string = path;
              }
            }
            
            // rjf: unpack arguments
            String8List passthrough_args_list = {0};
            for(String8Node *n = args.first->next; n != 0; n = n->next)
            {
              str8_list_push(scratch.arena, &passthrough_args_list, n->string);
            }
            StringJoin join = {str8_lit(""), str8_lit(" "), str8_lit("")};
            arguments_string = str8_list_join(scratch.arena, &passthrough_args_list, &join);
          }
          
          //- rjf: build config tree
          RD_Cfg *command_line_root = rd_cfg_child_from_string(rd_state->root_cfg, str8_lit("command_line"));
          RD_Cfg *target = rd_cfg_new(command_line_root, str8_lit("target"));
          RD_Cfg *exe    = rd_cfg_new(target, str8_lit("executable"));
          RD_Cfg *args   = rd_cfg_new(target, str8_lit("arguments"));
          RD_Cfg *wdir   = rd_cfg_new(target, str8_lit("working_directory"));
          rd_cfg_new(exe, executable_name_string);
          rd_cfg_new(args, arguments_string);
          rd_cfg_new(wdir, working_directory_string);
          
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
              RD_WindowState *dst_ws = rd_state->first_window_state;
              for(RD_WindowState *ws = dst_ws; ws != &rd_nil_window_state; ws = ws->order_next)
              {
                if(os_window_is_focused(ws->os))
                {
                  dst_ws = ws;
                  break;
                }
              }
              if(dst_ws != &rd_nil_window_state)
              {
                dst_ws->window_temporarily_focused_ipc = 1;
                U64 first_space_pos = str8_find_needle(msg, 0, str8_lit(" "), 0);
                String8 cmd_kind_name_string = str8_prefix(msg, first_space_pos);
                String8 cmd_args_string = str8_skip_chop_whitespace(str8_skip(msg, first_space_pos));
                RD_CmdKindInfo *cmd_kind_info = rd_cmd_kind_info_from_string(cmd_kind_name_string);
                if(cmd_kind_info != &rd_nil_cmd_kind_info) RD_RegsScope()
                {
                  if(dst_ws->cfg_id != rd_regs()->window)
                  {
                    Temp scratch = scratch_begin(0, 0);
                    RD_PanelTree panel_tree = rd_panel_tree_from_cfg(scratch.arena, rd_cfg_from_id(dst_ws->cfg_id));
                    rd_regs()->window = dst_ws->cfg_id;
                    rd_regs()->panel  = panel_tree.focused->cfg->id;
                    rd_regs()->view   = panel_tree.focused->selected_tab->id;
                    scratch_end(scratch);
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
      
      //- rjf: initializer pdb -> rdi conversion layer
      p2r_init();
      
      //- rjf: parse arguments
      P2R_User2Convert *user2convert = p2r_user2convert_from_cmdln(scratch.arena, cmd_line);
      
      //- rjf: open output file
      String8 output_name = push_str8_copy(scratch.arena, user2convert->output_name);
      OS_Handle out_file = os_file_open(OS_AccessFlag_Read|OS_AccessFlag_Write, output_name);
      B32 out_file_is_good = !os_handle_match(out_file, os_handle_zero());
      
      //- rjf: convert
      P2R_Convert2Bake *convert2bake = 0;
      if(out_file_is_good) ProfScope("convert")
      {
        convert2bake = p2r_convert(scratch.arena, user2convert);
      }
      
      //- rjf: bake
      P2R_Bake2Serialize *bake2srlz = 0;
      if(out_file_is_good) ProfScope("bake")
      {
        bake2srlz = p2r_bake(scratch.arena, convert2bake);
      }
      
      //- rjf: serialize
      P2R_Serialize2File *srlz2file = 0;
      if(out_file_is_good) ProfScope("serialize")
      {
        srlz2file = push_array(scratch.arena, P2R_Serialize2File, 1);
        srlz2file->bundle = rdim_serialized_section_bundle_from_bake_results(&bake2srlz->bake_results);
      }
      
      //- rjf: compress
      P2R_Serialize2File *srlz2file_compressed = srlz2file;
      if(out_file_is_good) if(cmd_line_has_flag(cmd_line, str8_lit("compress"))) ProfScope("compress")
      {
        srlz2file_compressed = push_array(scratch.arena, P2R_Serialize2File, 1);
        srlz2file_compressed = p2r_compress(scratch.arena, srlz2file);
      }
      
      //- rjf: serialize
      String8List blobs = {0};
      if(out_file_is_good)
      {
        blobs = rdim_file_blobs_from_section_bundle(scratch.arena, &srlz2file_compressed->bundle);
      }
      
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
                                    "This will step into all active targets after the debugger initially starts.\n\n"
                                    "--auto_run\n"
                                    "This will run all active targets after the debugger initially starts.\n\n"
                                    "--quit_after_success (or -q)\n"
                                    "This will close the debugger automatically after all processes exit, if they all exited successfully (with code 0), and ran with no interruptions.\n\n"
                                    "--ipc <command>\n"
                                    "This will launch the debugger in the non-graphical IPC mode, which is used to communicate with another running instance of the debugger. The debugger instance will launch, send the specified command, then immediately terminate. This may be used by editors or other programs to control the debugger.\n\n"));
    }break;
  }
  
  scratch_end(scratch);
}
