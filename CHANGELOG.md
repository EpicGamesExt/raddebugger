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
