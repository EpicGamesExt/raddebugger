# The RAD Debugger Project

The RAD Debugger is a native, user-mode, multi-process, graphical debugger. It
currently only supports local-machine Windows x64 debugging with PDBs, with
plans to expand and port in the future. In the future we'll expand to also
support native Linux debugging and DWARF debug info.

You can download pre-built binaries for the debugger
[here](https://github.com/EpicGames/raddebugger/releases).

The RAD Debugger project aims to simplify the debugger by simplifying and
unifying the underlying debug info format. In that pursuit we've built
the RADDBG debug info format, which is what the debugger parses and uses. To
work with existing toolchains, we convert PDB (and eventually PE/ELF files
with embedded DWARF) into the RADDBG format on-demand. This conversion process
is currently unoptimized but quite fast for smaller PDB files (in many cases
faster than many other programs simply deserialize the PDBs).

The RADDBG format is currently specified in code, in the files within the
`src/raddbg_format` folder. The other relevant folders for working with the
format are:

- `raddbg_cons`: The RADDBG construction layer, for constructing RADDBG files.
- `raddbg_convert`: Our implementation of PDB-to-RADDBG (and an in-progress
implementation of a DWARF-to-RADDBG) conversion.
- `raddbg_dump`: Code for textually dumping information from RADDBG files.

## Development Setup Instructions

**Note: Currently, only x64 Windows development is supported.**

### 1. Installing the Required Tools (MSVC & Windows SDK)

In order to work with the codebase, you'll need the [Microsoft C/C++ Build Tools
v15 (2017) or later](https://aka.ms/vs/17/release/vs_BuildTools.exe), for both
the Windows SDK and the MSVC compiler and linker.

If the Windows SDK is installed (e.g. via installation of the Microsoft C/C++
Build Tools), you may also build with [Clang](https://releases.llvm.org/).

### 2. Build Environment Setup

Building the codebase can be done in a terminal which is equipped with the
ability to call either MSVC or Clang from command line.

This is generally done by calling `vcvarsall.bat x64`, which is included in the
Microsoft C/C++ Build Tools. This script is automatically called by the `x64
Native Tools Command Prompt for VS <year>` variant of the vanilla `cmd.exe`. If
you've installed the build tools, this command prompt may be easily located by
searching for `Native` from the Windows Start Menu search.

You can ensure that the MSVC compiler is accessible from your command line by
running:

```
cl
```

If everything is set up correctly, you should have output very similar to the
following:

```
Microsoft (R) C/C++ Optimizing Compiler Version 19.29.30151 for x64
Copyright (C) Microsoft Corporation.  All rights reserved.

usage: cl [ option... ] filename... [ /link linkoption... ]
```

### 3. Building

Within this terminal, `cd` to the root directory of the codebase, and just run
the `build.bat` script:

```
build
```

You should see the following output:

```
[debug mode]
[msvc compile]
[default mode, assuming `raddbg` build]
metagen_main.c
searching C:\devel\raddebugger/src... 299 files found
parsing metadesk... 12 metadesk files parsed
gathering tables... 37 tables found
generating layer code...
raddbg.cpp
```

If everything worked correctly, there will be a `build` folder in the root
level of the codebase, and it will contain a freshly-built `raddbg.exe`.

## Top-Level Directory Descriptions

- `data`: Small binary files which are used when building, either to embed
  within build artifacts, or to package with them.
- `src`: All source code.

After setting up the codebase and building, the following directories will
also exist:

- `build`: All build artifacts. Not checked in to version control.
- `local`: Local files, used for local build configuration input files.

## Codebase Introduction

The codebase is organized into *layers*. Layers are separated either to isolate
certain problems, and to allow inclusion into various builds without needing to
pull everything in the codebase into a build. Layers correspond with folders
inside of the `src` directory. Sometimes, one folder inside of the `src`
directory will include multiple sub-layers, but the structure is intended to be
fairly flat.

Layers correspond roughly 1-to-1 with *namespaces*. The term "namespaces" in
this context does not refer to specific namespace language features, but rather
a naming convention for C-style namespaces, which are written in the codebase as
a short prefix, usually 1-3 characters, followed by an underscore. These
namespaces are used such that the layer to which certain code belongs may be
quickly understood by glancing at code. The namespaces are generally quite short
to ensure that they aren't much of a hassle to write. Sometimes, multiple sub-
layers will share a namespace. A few layers do not have a namespace, but most
do. Namespaces are either all-caps or lowercase depending on the context in
which they're used. For types, enum values, and some macros, they are
capitalized. For functions and global variables, they are lowercase.

Layers depend on other layers, but circular dependencies would break the
separability and isolation utility of layers (in effect, forming one big layer),
so in other words, layers are arranged into a directed acyclic graph.

A list of the layers in the codebase and their associated namespaces is below:
- `base` (no namespace): Universal, codebase-wide constructs. Strings, math,
  memory allocators, helper macros, command-line parsing, and so on. Depends
  on no other codebase layers.
- `coff` (`COFF_`): Code for parsing and/or writing the COFF (Common Object File
  Format) file format.
- `ctrl` (`CTRL_`): The debugger's "control system" layer. Implements
  asynchronous process control, stepping, and breakpoints for all attached
  processes. Runs in lockstep with attached processes. When it runs, attached
  processes are halted. When attached processes are running, it is halted.
  Driven by a debugger frontend on another thread.
- `dasm` (`DASM_`): An asynchronous disassembly decoder and cache. Users ask for
  disassembly for a particular virtual address range in a process, and threads
  implemented in this layer decode and cache the disassembly for that range.
- `dbgi` (`DBGI_`): An asynchronous debug info loader and cache. Loads debug
  info stored in the RADDBG format. Users ask for debug info for a particular
  executable, and on separate threads, this layer loads the associated debug
  info file. If necessary, it will launch a separate conversion process to
  convert original debug info into the RADDBG format.
- `demon` (`DEMON_`): An abstraction layer for local-machine, low-level process
  control. The abstraction is used to provide a common interface for process
  control on target platforms. Used to implement part of `ctrl`.
- `df/core` (`DF_`): The debugger's non-graphical frontend. Implements a
  debugger "entity cache" (where "entities" include processes, threads, modules,
  breakpoints, source files, targets, and so on). Implements a command loop
  for driving process control, which is used to implement stepping commands and
  user breakpoints. Implements extractors and caches for various entity-related
  data, like full thread unwinds and local variable maps. Also implements core
  building blocks for evaluation and evaluation visualization.
- `df/gfx` (`DF_`): The debugger's graphical frontend. Builds on top of
  `df/core` to provide all graphical features, including windows, panels, all
  of the various debugger interfaces, and evaluation visualization.
- `draw` (`D_`): Implements a high-level graphics drawing API for the debugger's
  purposes, using the underlying `render` abstraction layer. Provides high-level
  APIs for various draw commands, but takes care of batching them, and so on.
- `eval` (`EVAL_`): Implements a compiler for an expression language built for
  evaluation of variables, registers, and so on from debugger-attached processes
  and/or debug info. Broken into several phases mostly corresponding to
  traditional compiler phases - lexer, parser, type-checker, IR generation, and
  IR evaluation.
- `font_cache` (`F_`): Implements a cache of rasterized font data, both in CPU-
  side data for text shaping, and in GPU texture atlases for rasterized glyphs.
  All cache information is sourced from the `font_provider` abstraction layer.
- `font_provider` (`FP_`): An abstraction layer for various font file decoding
  and font rasterization backends.
- `geo_cache` (`GEO_`): Implements an asynchronously-filled cache for GPU
  geometry data, filled by data sourced in the `hash_store` layer's cache. Used
  for asynchronously preparing data for memory visualization in the debugger.
- `hash_store` (`HS_`): Implements a cache for general data blobs, keyed by a
  128-bit hash of the data. Used as a general data store by other layers.
- `metagen` (`MG_`): A metaprogram which is used to generate primarily code and
  data tables. Consumes Metadesk files, stored with the extension `.mc`, and
  generates C code which is then included by hand-written C code. Currently, it
  does not analyze the codebase's hand-written C code, but in principle this is
  possible. This allows easier & less-error-prone management of large data
  tables, which are then used to produce e.g. C `enum`s and a number of
  associated data tables. There are also a number of other generation features,
  like embedding binary files or complex multi-line strings into source code.
  This layer cannot depend on any other layer in the codebase directly,
  including `base`, because it may be used to generate code for those layers. To
  still use `base` and `os` layer features in the `metagen` program, a separate,
  duplicate version of `base` and `os` are included in this layer. They are
  updated manually, as needed. This is to ensure the stability of the
  metaprogram.
- `mule` (no namespace): Test executables for battle testing debugger
  functionality.
- `natvis` (no namespace): NatVis files for type visualization of the codebase's
  types in other debuggers.
- `os/core` (`OS_`): An abstraction layer providing core, non-graphical
  functionality from the operating system under an abstract API, which is
  implemented per-target-operating-system.
- `os/gfx` (`OS_`): An abstraction layer, building on `os/core`, providing
  graphical operating system features under an abstract API, which is
  implemented per-target-operating-system.
- `os/socket` (`OS_`): An abstraction layer, building on `os/core`, providing
  networking operating system features under an abstract API, which is
  implemented per-target-operating-system.
- `pe` (`PE_`): Code for parsing and/or writing the PE (Portable Executable)
  file format.
- `raddbg` (no namespace): The layer which ties everything together for the main
  graphical debugger. Not much "meat", just drives `df`, implements command line
  options, and so on.
- `raddbg_cons` (`CONS_`): Implements an API for constructing files of the
  RADDBG debug info file format.
- `raddbg_dump` (`DUMP_`): A dumper utility program for dumping textualizations
  of RADDBG debug info files.
- `raddbg_format` (`RADDBG_`): Standalone types and helper functions for the
  RADDBG debug info file format. Does not depend on `base`.
- `raddbg_markup` (`RADDBG_`): Standalone header file for marking up user
  programs to work with various features in the `raddbg` debugger. Does not
  depend on `base`.
- `regs` (`REGS_`): Types, helper functions, and metadata for registers on
  supported architectures. Used in reading/writing registers in `demon`, or in
  looking up register metadata.
- `render` (`R_`): An abstraction layer providing an abstract API for rendering
  using various GPU APIs under a common interface. Does not implement a high
  level drawing API - this layer is strictly for minimally abstracting on an
  as-needed basis. Higher level drawing features are implemented in the `draw`
  layer.
- `scratch` (no namespace): Scratch space for small and transient test or sample
  programs.
- `texture_cache` (`TEX_`): Implements an asynchronously-filled cache for GPU
  texture data, filled by data sourced in the `hash_store` layer's cache. Used
  for asynchronously preparing data for memory visualization in the debugger.
- `txti` (`TXTI_`): Machinery for asynchronously-loaded, asynchronously hot-
  reloaded, asynchronously parsed, and asynchronously mutated source code files.
  Used by the debugger to visualize source code files. Users ask for text lines,
  tokens, and metadata, and it is prepared on background threads.
- `type_graph` (`TG_`): Code for analyzing and navigating type structures from
  RADDBG debug info files, with the additional capability of constructing
  synthetic types *not* found in debug info. Used in `eval` and for various
  visualization features.
- `ui` (`UI_`): Machinery for building graphical user interfaces. Provides a
  core immediate mode hierarchical user interface data structure building
  API, and has helper layers for building some higher-level widgets.
- `unwind` (`UNW_`): Code for generating unwind information from threads, for
  supported operating systems and architectures.
