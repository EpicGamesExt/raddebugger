# v0.9.29-alpha

## Debugger Changes
- Reintroduced `Browse` buttons for cells that edit file paths, which spawn a
  file picker, to more conveniently choose paths.
- The debugger now tags watch expressions with the selected project, which
  means they'll be loaded and unloaded along with projects, similar to targets
  and breakpoints.
- Empty spaces in the leftmost margin (where the selected thread position is
  visualized) in source and disassembly views is now clickable, as a fastpath
  for `Run To Line`.
- The position of the destination of a `Run To Line` command is now visualized.

# v0.9.28-alpha

## Debugger Changes
- First pass (will only work for simple cases currently) of watch expressions
  locks, allowing the same address (with the same type) to be evaluated after
  the expression goes out of scope.
- Improved disassembly snapping behavior when not viewing disassembly. Now, if
  you do not have disassembly open, the debugger will prefer snapping to a call
  stack frame which has line info.	
- Added some visual and discoverability improvements to the UI.
- Fixed a crash in the new (as of 0.9.27) unwinder. (#893, #882, #877, #869,
  #867, #865, #857, #848, #855, #824, #823)
- Fixed a number of memory leaks, causing out-of-control memory usage in some
  cases. (#850)
- Fixed certain cases of location information being incorrectly generated
  (notably in optimized builds, and functions with large stack usage). (#864)
- Fixed incorrect session start logic (debug string clearing, breakpoint hit
  count resets) in multi-process debugging scenarios.
- Fixed rare issues with call stack computation reliability.
- Fixed cursor trails not being toggled by the `Animations` setting.
- Fixed cursor trails being incorrectly rendered when a cursor is first
  scrolled into view.
- Fixed line edits embedded in cells (e.g. in the Palette) not correctly
  clipping and scrolling longer strings. (#873)
- Fixed behavior of setting the project path when that would overwrite a file
  which is not a debugger project file. (#875)

## Linker Changes
- Implemented `/OPT:ICF` identical COMDAT folding.
- Added DEF, and SECTION configuration support.
- Improved COMDAT handling for weak symbols, ICF, and zero-sized COMDAT
  anchors, and stopped emitting empty marker sections that could produce
  invalid PE images.
- Fixed import/export edge cases, including import data-directory bounds and
  archive member handling for `__imp_*` symbols.
- Accepted duplicate identical weak search aliases without reporting multiply
  defined symbols.
- Add support for UTF-16 encoded response files.
- Fixed DBI section-contribution image-section range mapping.
- Improved debug logging for `/OPT:REF` liveness stats and unresolved symbols
  referenced by `.llvm_addrsig`.
- Improved diagnostics for unresolved symbols: radlink now identifies functions
  that reference undefined symbols, even if the function was inlined (provided
  the object file was compiled with debug info)
- Reduced memory usage for large links.
- Fixed forwarder export detection when the symbol name contains dots and has
  no alias.

# v0.9.27-alpha

## Debugger Changes

- Fixed an issue which would rarely cause steps to take a long time.
- Drastically improved memory usage of PDB -> RDI conversion for some
  workloads, notably PDBs with many small compilation units.
- Added the ability to disambiguate symbol names by module name, debug info
  name, or compilation unit name in expressions. This can be done using the
  usual syntax, e.g. `my_module.dll!foo`, or `my_unit.obj!foo`. `:` can also be
  used instead of `!`.
- Added the ability to redirect debug string output to any label on a
  per-target basis. Those labels can then be evaluated in any watch window or
  text visualizer. By default, all targets write to `output`, which is
  automatically visualized by an `Output` tab. But if you want different
  targets to route their debug output to different buffers, you can name the
  buffers differently, and then view those in separate text views.
- Added a `Run To Name` command.
- Added an `Auto Load Last Project` setting, which will automatically load the
  most recently loaded project on startup (the old behavior pre-0.9.26). This
  setting is off by default.
- Added right-click menus for watch expressions, to easily find them in a
  memory view, or to break when their value changes.
- Added preliminary support for symbol servers. First, the debugger can attempt
  to download debug info for any module via the `Download Module Debug Info`
  command (also accessible by right-clicking a module). Second, the debugger
  can be configured to automatically do this for all modules which don't have
  debug info via the `Auto Download Debug Info` setting. The local cache path
  and server URLs used by the debugger for downloading symbols are parsed from
  the `_NT_SYMBOL_PATH` environment variable. If that environment variable is
  not configured, then it falls back to the defaults of `C:/SymbolCache` and
  `https://msdl.microsoft.com/download/symbols`.
- Adjusted the default dark theme.
- Made several UI visual improvements.
- Fixed a few issues relating to offline symbol evaluation (evaluation of
  symbols in debug info when not debugging); this was sometimes preventing
  function listers (e.g. used in `Go To Name`) from working when not debugging.
- Fixed the debugger's snap-to-code-location rule when a `Disassembly` tab is
  occupying the same panel as source code tabs. Previously, the debugger would
  overly aggressively switch back to source code tabs. Now, it should respect
  the preference of disassembly correctly.
- Improved the stability of the step-over-line operation. Our stepping
  algorithm would sometimes cause spurious (and rarely, non-spurious)
  access violation exceptions when a thread would try to execute at address
  `0x38f`. This most regularly surfaced when an exception on one thread (like a
  breakpoint being hit) would interrupt stepping on another thread. This has
  been resolved, and stepping in such scenarios should be much more reliable.
- Fixed some crashes in the PDB -> RDI converter. (#792, #803)
- Fixed a bug which was sometimes causing a step-over operation to resume the
  process without catching the target thread after its step completed. (#804)
- Adjusted evaluation visualizations to include fully qualified symbol names.
  (#809)

## Linker Changes
- /OPT:REF Fixed a race in the dead-code-elimination pass where the linker
  sometimes erroneously removed live sections.
- Fixed a bug where the linker emitted relocations for empty COFF resource entries,
  which lead to "relocating against symbol that is in a removed section" errors.
- Added the /RAD_TYPE_SERVER switch to run the type-dedup pass on object files,
  producing RRT files that can be passed back on the next link to improve
  linking speed.

# v0.9.26-alpha

## Debugger Changes

- Added scope ending visualizations to the text view, to display the header of
  a scope at the closing position of the scope. This can be turned off via the
  `Cursor Scope End Annotations` setting.
- Added the ability to evaluate `autos`, which is a collection of expressions
  that the debugger determines refer to symbols which are being (or about to
  be) changed by the selected thread.
- Added source-inline and disassembly-inline visualization of auto expressions.
  This can be turned off via the `Show Auto Watches In Source / Disassembly`
  setting.
- Added the ability to evaluate local static variables unambiguously, when the
  same name is used for multiple local static variables, even when outside the
  function containing the local static. This can be done by namespacing the
  variable name with the containing function, either using a `::` or `.`
  operator. (#359)
- Added a setting, `Transient Tabs`, to disable transient tabs being opened,
  when the debugger automatically snaps to new code locations.
- Added a setting, `Display Pointer Addresses Before Contents`, to always
  force the debugger to show pointer addresses before attempting to visualize
  to what the pointer points.
- Adjusted the `Step Out` command to perform scope-granularity stepping, rather
  than only being function-frame-granularity. This uses scope information found
  in debug info to step out of scopes when possible, rather than always exiting
  the current function frame. This makes the command much more useful for
  stepping through larger functions with several scopes of larger work, or for
  stepping out of loops. (#746)
- Adjusted the debugger's user & project configuration file behavior to be less
  idiosyncratic with other programs. Project files are no longer auto-loaded
  based on the project last loaded during the previous debugger runtime.
  Creating a new user or project also does not immediately require choosing a
  path for the file; these can later be specified using the respective `Save`
  commands. (#743)
- Adjusted the debugger to automatically set the current path when loading a
  project file. (#743)
- Added syntax highlighting and tab-completion for view identifiers in
  watch expressions.
- Added more in-debugger documentation for views and their arguments, when
  written in a watch expression.
- Moved configuration data previously treated as project data to being user
  data, tagged with the associated project. This includes loaded debug info and
  breakpoints. The effect of this is that project files are much easier to
  check into source control (they do not change in spurious ways for other
  users on a project), but project-related user configuration will still be
  correctly filtered based on whatever project is loaded.
- Improved the debugger's behavior with respect to keeping previously-loaded
  debug info around. Now, when the debugger begins a new debugging session, it
  will unload all loaded debug info, and it will only reload debug info that is
  actively found to be needed by debuggees. This preserves the debugger's
  ability to use debug info without debugging for features like
  go-to-definition and type evaluation, but it stops the debugger from keeping
  old and unneeded debug info loaded indefinitely (until it is manually
  unloaded). (#711)
- Adjusted recent project visualization to include the name of the project,
  specified with the `Project Name` setting.
- Renamed `Switch` to `Open Source File From Debug Info`, to better represent
  the command's functionality.
- Adjusted file path evaluation visualization to include the full path to the
  containing folder, when it is not obvious from context (for instance, if the
  file evaluation is a child evaluation of a folder evaluation, the containing
  folder path is not shown; if the file path is evaluated standalone, as it is
  in the lister generated for the `Open Source File From Debug Info` command,
  then the containing folder path is shown).
- Improved visualization of unmapped addresses in the memory view. Now, instead
  of bytes at these addresses being displayed as `0` with a special background,
  they are displayed as `x`, and the annotations for these addresses (displayed
  at the bottom of the memory view, for the cursor's cell) explicitly label
  them as being unmapped.
- Improved visualization of unmapped addresses in watch expressions. Now,
  instead of only being colored differently with evaluations reading as zeroed
  memory, the value will explicitly label addresses as unmapped when possible.
- Fixed a leak which most notably manifested during rapid creation/destruction
  of new threads. (#780)
- Fixed a leak related to debugger's config state. This was difficult to
  reproduce, but it may fix leaks experienced after running the debugger for an
  extended period of time.
- Fixed a bug where cross-module evaluations would resolve to incorrect
  addresses, due to using the incorrect module's base virtual address. This was
  also causing broken behavior sometimes in the `Go To Name` lister, and
  other similar UIs.
- Fixed a bug where panels without tabs were unnecessarily keeping the debugger
  UI awake in some cases.
- Fixed a bug preventing hover evaluation from working on indexing expressions.
  (#707)
- Replaced the DWARF -> RDI converter with a new multithreaded version, which
  should execute much faster on larger DWARF debug info, and produce much
  smaller RDI output, due to the addition of type deduplication.
- Introduced structured namespaces to RDI, which structurally encode namespace
  hierarchies for debug info symbols, instead of this information being
  implicitly baked using language-specific conventions into symbol names. This
  is used to deduplicate repeated namespace prefixes from symbol names, leading
  to a smaller string footprint for generated RDIs. Symbols now only store
  their partially-qualified name. For example, for a C++ symbol `A::B::C`, the
  symbol itself will store its name as `C`, but it will also record its
  container as being `B`. `B`, then, will record its container as being `A`.
  This allows users of RDI to dynamically construct the fully-qualified name as
  needed, and it allows for more reliable and language-agnostic usage of
  partially-qualified symbol names.
- Adjusted the debugger to not rely on `SynchronizationBarrier` functions on
  Windows when they're not available (pre-Windows-8). (#739)


## Linker Changes

- Implemented /DEBUG:GHASH
- Fixed an issue where an import thunk or import address could be linked from
  the currently searched library instead of the library where the first import
  reference was found. For example, if the first pass finds the import thunk in
  foo.lib and the second pass resolves symbol to the import address in bar.lib,
  the linker now always linkes the import thunk and import address members from
  the library where the first reference was found.
- /OPT:REF:  Fixed an issue with garbage collection of associated sections and
  weak symbols that led linker to remove live sections.
- /OPT:REF Added relocation batching to reduce contention on the global
  reference list during garbage collection.

# v0.9.25-alpha

## Debugger Changes

- Added a dedicated cursor address bar to the memory view. This can be focused
  with the keyboard with the `Focus Menu` command. By default, this command is
  bound to `Alt + D`, but if you have existing configuration, you may need to
  bind it yourself manually (this can be done in the palette). This address bar
  accepts any expression, and controls the cursor's address. All other
  configuration options are still available in the memory view settings.
- Added "Peek Types" to the memory view. These control the set of types which
  are used to interpret bytes at the cursor. Options exist for basic cases,
  like integers and floats, but the view also supports entering a list of
  full type expressions, which can include user-defined types like structures.
- Added annotations for members of structure variables to the memory view.
- Added a dedicated zoom setting for the memory view.
- Added a setting for automatically determining the number of columns in the
  memory view based on the available space.
- Fixed many bugs and improved visuals in the memory view. (#690)
- Fixed the debugger incorrectly evaluating pointer casts of registers in
  register space, rather than promoting them to process address space
  evaluations. This fixes cases where expressions like `(int *)rax` were not
  evaluating correctly. (#659)
- Fixed the debugger incorrectly requiring multiple step operations when source
  lines mapped to multiple discontiguous ranges of instructions. This most
  notably showed up with certain styles of `for`-loops, especially with Clang
  and other non-C/C++ languages, and function prologues in non-C/C++ languages.
- Fixed the debugger treating the `foo, x` fastpath for `hex(foo)` differently
  than `hex(foo)`, in that the hexadecimal setting was not being inherited.
- Fixed the debugger leaking debug info conversion process handles. This
  addresses cases where the debugger was preventing stale debug info from being
  updated, after a rebuild. (#736)
- Fixed the debugger not correctly using target-embedded markup data. (#702)
- Fixed the debugger not showing full call stacks when the top instruction
  pointer is located at a 0 address.
- Fixed integer casts producing incorrect values. (#754)
- Fixed the PDB -> RDI converter's interpretation of certain empty argument
  lists in function types in PDB. This fixes some cases where local variables
  were being interpreted as parameters, which resulted in incorrect evaluation
  results. (#751)
- Fixed several crashes and instabilities. (#735)
- Added a dedicated settings button to the focused tab. This opens the same
  menu as right-clicking the tab, but this makes this menu (and thus all
  options for all tabs and views) more discoverable.
- Added optional cursor trails, to visualize cursor motion. Can be turned off
  by toggling the `Cursor Trail` setting.
- Added project names. When set, this name is used for displaying the project
  instead of the project filename in the UI and window titles.
- Adjusted the rules for explicit file location finding commands (like
  `Switch`, or `Switch To Partner File`), such that they prefer creating new
  non-transient tabs, even if the target file is found within an existing
  transient tab.
- Adjusted the source code context menu to activate commands with a single
  press, rather than the usual double click behavior from general palettes.

## Linker Changes

- Optimized debug info generation to reduce memory usage in large links by 25%.
- Fixed a determinism issue that caused linker to write unreliable debug info.
- Fixed over-reservetaion of PDB pages, reducing the PDB size. 
- Implemented /INFERASANLIBS
- Implemented /WHOLEARCHIVE
- Implemented /PDBSTRIPPED
- Set correct defaults for unload and bind import tables.

# v0.9.24-alpha

## Debugger Changes

- Added the ability for the debugger to load, use, and evaluate using debug
  info, even when not actively debugging. The debugger will now keep a process'
  debug info loaded, even after the process ends. It stores the set of loaded
  debug info files in the project configuration file, meaning it will also
  automatically load the same debug info across many runs. Debug info can also
  be loaded manually (without ever launching a process) with the
  `Load Debug Info` command. There is also a new tab, `Debug Info`, which allows
  viewing and managing the set of loaded debug info files.
- Improved the debugger's behavior when used as a drag & drop target, to allow
  for debug info loading as an option (when relevant), and to better handle the
  case where many files (potentially of different types) are dropped together.
- Improved debug info searching performance and reponsiveness in large projects.
- Fixed some crashes and incorrect results with the new `list` view.
- Fixed some cases where RDIs did not contain some basic types from their
  originating PDBs.
- Allowed `.` and `->` operators to be used with array types.
- Fixed the debugger's treatment of quoted command line arguments when building
  targets. In previous versions, calling `raddbg main.exe "foo bar baz"` would
  create a target `main.exe` with arguments `foo bar baz` (dropping the quotes).
  This is now fixed, such that the target's arguments string will also contain
  the quotes, and pass them to the target when launched.
- Fixed the debugger not correctly responding (through font and UI scale) to DPI
  changes.
- Fixed the debugger incorrectly generating conflicting source line info records
  in PDB -> RDI conversion, which in some scenarios was preventing source line
  maps from working (leading to breakpoint resolution failing).
- Other small fixes, improvements, and tweaks.

# v0.9.23-alpha

## Debugger Changes

- Further improved PDB -> RDI conversion performance & memory usage.
- Adjusted limits for the amount of PDB -> RDI conversion work that the
  debugger will kick off, to prevent PDB -> RDI conversion interfering with
  debuggee performance.
- Reintroduced the `list` lens, which gathers all nodes in a linked list, and
  visualizes them as a flat list of pointers (the same as how an array of
  pointers is visualized).
- Fixed a crash when closing empty Geometry 3D views. (#652, #657)
- Fixed the debugger not visualizing `enum` types when evaluated through a
  bitfield type. (#655)
- Fixed the PDB -> RDI conversion not correctly generating location information
  for function parameters, when the EXE/PDB were built to include support for
  Edit and Continue (`-ZI` switch). (#656)

## Binary Utility Changes

- Fixed a crash when using the `--compress` option, when generating RDI files.

# v0.9.22-alpha

## Debugger Changes

- Further improved PDB -> RDI conversion performance.
- Capped the number of additional threads / processes spawned for PDB -> RDI
  conversion.
- Prioritize PDB -> RDI conversion based on what is actually found to be
  necessary by the debugger, rather than converting all PDBs in the order in
  which they're discovered.
- Added preliminary support for DWARF -> RDI conversion on Windows.
- The debugger now relies on source file checksums to determine whether or not
  a source file is out-of-date with respect to what was compiled when debug info
  was produced, rather than just the modification timestamp.
- The debugger now will rely on debug info to detect the language of source code
  files, if it cannot infer from the source file's extension, or view settings.
  This will enable features like syntax highlighting and hover evaluation in
  cases like `.inl` files being included in C++ projects.
- The debugger now will restore the last focused window when continuing
  automatically. (#245, #596)
- Watch tables have been simplified in that they no longer have a separate
  column for evaluation types, since this was usually taking a lot more space
  than it deserved. The type of evaluations is still displayed in watch table
  cells, and it can always be evaluated directly via `typeof`.
- Type evaluations have been simplified in watch tables as well; they no longer
  have untitled columns for sizes and offsets, this is instead displayed as
  an extra note by default. Similar behavior to the original behavior can still
  be obtained using the `columns` view, if needed.
- The debugger no longer uses complex `union` types for most registers, and
  instead just displays the register value plainly.
- The hover evaluation UI has been made larger when needed.
- The debugger now prefers matching global, function, and type identifiers to
  the most relevant debug info and module in context; this fixes evaluation in
  some multi-process debugging contexts. (#581)
- Fixed the debugger unnecessarily stripping `enum` type information when
  accessed through array operators. (#634)
- The debugger now understands a standalone `unsigned` keyword as an
  `unsigned int` type, to match C rules.
- The debugger now uses the current working directory to form the working
  directory for targets specified on the command line, to match behavior when
  running a command from the command line without the debugger.
- Improved call stack computation performance.
- Improved debugger memory usage over long periods of time.
- Fixed string-pointer comparison not working with not-equal (`!=`) operations.
- Fixed a bug which was causing bad debuggee performance on some threads after
  some interactions with the debugger controller.
- Fixed incorrect results when adding two register values. (#642)
- Fixed the interpretation of register expressions in visualizers. (#649)
- Fixed "forever loading" states in disassembly views in some cases. (#643)
- Fixed jittering on window resizing. (#636)
- Fixed the bitmap visualizer crashing in some circumstances relating to
  unsupported bitmap sizes. (#444, #563)
- Fixed a crash when an empty `cast()` expression would be evaluated. (#625)
- Fixed a crash when an invalid expression would be visualized using the `text`
  view. (#647)

## Linker Changes

- Changed symbol resolution in libaries to match MSVC behavior.
- Optimized image building step to reduce memory usage.
- Linker memory maps all input files by default to lower memory usage.
  (`/RAD_MEMORY_MAP_FILES`)
- If debug info is available, linker uses it to show file and line number for
  unresolved relocations.
- Improved base relocation build performance for large images, cutting build
  time by 70%.
- Added stubs for `/Brepro`, `/D2`, and /ErrorReport to improve compatability
  with existing response files
- Implemented section garbage collection (`/OPT:REF`)
- Fixed bug where thread local variables pointed to incorrect types.
- Changed rules for weak and undefined symbols, now weak symbol is not allowed
  to replace an undefined symbol.
- Linker no longer creates thunks for imports that don't require them.

## Binary Utility Changes

- The binary utility, like the debugger, now can convert DWARF debug info to
  RDI files. When both DWARF and PDB info is present, it can now convert both,
  and produce a single final RDI file with all information.
- Textual dumping of RDI files is now done in parallel, massively improving
  dumping performance.
- PDB -> Breakpad conversion performance has now been parallelized to a greater
  degree, improving performance.
