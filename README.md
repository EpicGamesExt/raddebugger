# The RAD Debugger Project

_**Note:** This README does not document usage instructions and tips for the
debugger itself, and is intended as a technical overview of the project. The
debugger's README, which includes usage instructions and tips, can be found
packaged along with debugger releases, or within the `build` folder after a
local copy has been built._

The RAD Debugger is a native, user-mode, multi-process, graphical debugger. It
currently only supports local-machine Windows x64 debugging with PDBs, with
plans to expand and port in the future. In the future we'll expand to also
support native Linux debugging and DWARF debug info.

The RAD Debugger is currently in *ALPHA*. In order to get the debugger bullet-
proof, it'd greatly help out if you submitted the issues you find here, along
with any information you can gather, like dump files (along with the build you
used), instructions to reproduce, test executables, and so on.

Dump files can be generated when ``RadDdg.exe`` crashes by opening task manager,
right clicking on the ``RadDbg.exe`` process and selecting "Create Memory Dump from File".

You can download pre-built binaries for the debugger
[here](https://github.com/EpicGamesExt/raddebugger/releases).

The RAD Debugger project aims to simplify the debugger by simplifying and
unifying the underlying debug info format. In that pursuit we've built the RAD
Debug Info (RDI) format, which is what the debugger parses and uses. To work
with existing toolchains, we convert PDB (and eventually PE/ELF files with
embedded DWARF) into the RDI format on-demand.

The RDI format is currently specified in code, in the files within the
`src/lib_rdi_format` folder. The other relevant folders for working with the
format are:

- `lib_rdi_make`: The "RAD Debug Info Make" library, for making RDI debug info.
- `rdi_from_pdb`: Our PDB-to-RDI converter. Can be used as a helper codebase
  layer, or built as an executable with a command line interface frontend.
- `rdi_from_dwarf`: Our in-progress DWARF-to-RDI converter.
- `rdi_dump`: Our RDI textual dumping utility.

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
metagen_main.c
searching C:\devel\raddebugger/src... 309 files found
parsing metadesk... 15 metadesk files parsed
gathering tables... 96 tables found
generating layer code...
raddbg_main.c
```

If everything worked correctly, there will be a `build` folder in the root
level of the codebase, and it will contain a freshly-built `raddbg.exe`.

## Short-To-Medium-Term Roadmap

### The Initial Alpha Battle-Testing Phase

The first priority for the project is to ensure that the most crucial debugger
components are functioning extremely reliably for local, x64, Windows
debugging. This would include parts like debug info conversion, debug info
loading, process control, stepping, evaluation (correct usage of both location
info and type info), and a robust frontend which ensures the lower level parts
are usable.

We feel that the debugger has already come a long way in all of these respects,
but given the massive set of possible combinations of languages, build
settings, toolchains, used language features, and patterns of generated code,
there are still cases where the debugger has not been tested, and so there are
still issues. So, we feel that the top priority is eliminating these issues,
such that the debugging experience is rock solid.

### Local x64 Linux Debugging Phase

The next priority for the project is to take the rock solid x64 Windows
debugging experience, and port all of the relevant pieces to support local x64
Linux debugging also.

The debugger has been written to abstract over the parts that need to differ on
either Linux or Windows, and this is mainly going to be a task in building out
different backends for those abstraction layers.

The major parts of this phase are:

- Porting the `src/demon` layer to implement the Demon local process control
abstraction API.
- Implementing an x64 ELF Linux unwinder in the `src/ctrl` layer.
- Creating a DWARF-to-RDI converter (in the same way that we've built a
PDB-to-RDI converter). A partial implementation of this is in
`src/rdi_from_dwarf`.
- Porting the `src/render` layer to implement all of the rendering features the
frontend needs on a Linux-compatible API (the backend used on Windows is D3D11).
- Porting the `src/font_provider` layer to a Linux-compatible font
rasterization backend, like FreeType (the backend used on Windows is
DirectWrite).
- Porting the `src/os` layers to Linux. This includes core operating system
abstraction (virtual memory allocation, threading and synchronization
primitives, and so on), and graphical operating system abstraction (windows,
input events, and so on).

Once the above list is complete, and once every part is rock solid, the Windows
debugging experience we'll have worked diligently to create will also be
available natively on Linux machines.

### And Beyond!

There are several directions we might take after these two major phases,
like remote debugging, porting to different architectures, further improving
the debugger's features (like improving the visualization engine), and so on.
But for now, we're mostly focused on those first two phases.

---

# The RAD Linker

The RAD Linker is a new performance linker for generating x64 PE/COFF binaries. It is designed to be very fast when creating gigantic executables. It generates standard PDB files for debugging, but it can also optionally create RAD Debugger debug info too (useful for huge executables that otherwise create broken PDBs that overflow internal 32-bit tables).

The RAD Linker is primarily optimized to handle huge linking projects - in our test cases (where debug info is multiple gigabytes), we see 50% faster link times. 

The command line syntax is fully compatible with MSVC and you can get a full list of implemented switches from `/help`.

Our current designed-for use case for the linker is to help with the compile-debug cycle of huge projects. We don't yet have support for dead-code-elimination or link-time-optimizations, but these features are on the road map.

By default, the RAD linker spawns as many threads as there are cores, so if you plan to run multiple linkers in parallel, you can limit the number of thread workers via `/rad_workers`.

We also have support for large memory pages, which, when enabled, reduce link time by
another 25%. To link with large pages, you need to explicitly request them via `/rad_large_pages`. Large pages are off by default, since Windows support for large pages is a bit buggy - we recommend they only be used in Docker or VM images where the environment is reset after each link. In a standard Windows environment, using large pages otherwise will fragment memory quickly forcing a reboot. We are working on a Linux port of the linker that will be able to build with large pages robustly.

## Short Term Roadmap
- Porting linker to Linux (for Windows executables, just running on Linux).
- Debug info features
  - Get DWARF debug info converter up-and-running.
  - Smooth out rough edges in RADDBGI builder.
  - Improve build speed further (especially for tiny and mid sizes projects).
- Other features to come
  - Dead-code-elimination via `/opt:ref`.
  - Link Time Optimizations with the help of clang (we won't support LTCG from MSVC compiler since it is undocumented).

## To build the RAD Linker
- Setup development environment, [see](#Development-Setup-Instructions)
- Run `build radlink release` or if you have clang installed `build radlink release clang`. We favor latter option for better code generation.

If build was successful linker executable is placed in `build` folder under `radlink.exe`.

## Benchmarks

![AMD Ryzen Threadripper PRO 3995WX 64-Cores, 256 GiB RAM (Windows x64)](https://github.com/user-attachments/assets/a95b382a-76b4-4a4c-b809-b61fe25e667a)

---

## Top-Level Directory Descriptions

- `data`: Small binary files which are used when building, either to embed
  within build artifacts, or to package with them.
- `src`: All source code.

After setting up the codebase and building, the following directories will
also exist:

- `build`: All build artifacts. Not checked in to version control.
- `local`: Local files, used for local build configuration input files. Not
  checked in to version control.

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

A few layers are built to be used completely independently from the rest of the
codebase, as libraries in other codebases and projects. As such, these layers do
not depend on any other layers in the codebase. The folders which contain these
layers are prefixed with `lib_`, like `lib_rdi_format`.

A list of the layers in the codebase and their associated namespaces is below:
- `async` (`ASYNC_`): Implements a system for asynchronous work to be queued
  and executed on a thread pool.
- `base` (no namespace): Universal, codebase-wide constructs. Strings, math,
  memory allocators, helper macros, command-line parsing, and so on. Depends
  on no other codebase layers.
- `codeview` (`CV_`): Code for parsing and/or writing the CodeView format.
- `coff` (`COFF_`): Code for parsing and/or writing the COFF (Common Object File
  Format) file format.
- `ctrl` (`CTRL_`): The debugger's "control system" layer. Implements
  asynchronous process control, stepping, and breakpoints for all attached
  processes. Runs in lockstep with attached processes. When it runs, attached
  processes are halted. When attached processes are running, it is halted.
  Driven by a debugger frontend on another thread.
- `dasm_cache` (`DASM_`): An asynchronous disassembly decoder and cache. Users
  ask for disassembly for some data, with a particular architecture, and other
  various parameters, and threads implemented in this layer decode and cache the
  disassembly for that data with those parameters.
- `dbgi` (`DI_`): An asynchronous debug info loader and cache. Loads debug info
  stored in the RDI format. Users ask for debug info for a particular path, and
  on separate threads, this layer loads the associated debug info file. If
  necessary, it will launch a separate conversion process to convert original
  debug info into the RDI format.
- `dbg_engine` (`D_`): Implements the core debugger system, without any
  graphical components. This contains top-level logic for things like stepping,
  launching, freezing threads, mid-run breakpoint addition, some caching layers,
  and so on.
- `demon` (`DMN_`): An abstraction layer for local-machine, low-level process
  control. The abstraction is used to provide a common interface for process
  control on target platforms. Used to implement part of `ctrl`.
- `draw` (`DR_`): Implements a high-level graphics drawing API for the
  debugger's purposes, using the underlying `render` abstraction layer. Provides
  high-level APIs for various draw commands, but takes care of batching them,
  and so on.
- `eval` (`E_`): Implements a compiler for an expression language built for
  evaluation of variables, registers, types, and more, from debugger-attached
  processes, debug info, debugger state, and files. Broken into several phases
  mostly corresponding to traditional compiler phases - lexer, parser,
  type-checker, IR generation, and IR evaluation.
- `eval_visualization` (`EV_`): Implements the core non-graphical evaluation
  visualization engine, which can be used to visualize evaluations (provided by
  the `eval` layer) in a number of ways. Implements core data structures and
  transforms for the `Watch` view.
- `file_stream` (`FS_`): Provides asynchronous file loading, storing the
  artifacts inside of the cache implemented by the `hash_store` layer, and
  hot-reloading the contents of files when they change. Allows callers to map
  file paths to data hashes, which can then be used to obtain the file's data.
- `font_cache` (`FNT_`): Implements a cache of rasterized font data, both in
  CPU-side data for text shaping, and in GPU texture atlases for rasterized
  glyphs. All cache information is sourced from the `font_provider` abstraction
  layer.
- `font_provider` (`FP_`): An abstraction layer for various font file decoding
  and font rasterization backends.
- `fuzzy_search` (`FZY_`): Provides a fuzzy searching engine for doing
  large, asynchronous fuzzy searches. Used by the debugger for implementing
  things like the symbol lister or the `Procedures` view, which search across
  all loaded debug info records, using fuzzy matching rules.
- `geo_cache` (`GEO_`): Implements an asynchronously-filled cache for GPU
  geometry data, filled by data sourced in the `hash_store` layer's cache. Used
  for asynchronously preparing data for visualization.
- `hash_store` (`HS_`): Implements a cache for general data blobs, keyed by a
  128-bit hash of the data. Also implements a 128-bit key cache on top, where
  the keys refer to a unique identity, associated with a 128-bit hash, where the
  hash may change across time. Used as a general data store by other layers.
- `lib_raddbg_markup` (`RADDBG_`): Standalone library for marking up user
  programs to work with various features in the debugger. Does not depend on
  `base`, and can be independently relocated to other codebases.
- `lib_rdi_format` (`RDI_`): Standalone library which defines the core RDI types
  and helper functions for reading and writing the RDI debug info file format.
  Does not depend on `base`, and can be independently relocated to other
  codebases.
- `lib_rdi_make` (`RDIM_`): Standalone library for constructing RDI debug info
  data. Does not depend on `base`, and can be independently relocated
  to other codebases.
- `mdesk` (`MD_`): Code for parsing Metadesk files (stored as `.mdesk`), which
  is the JSON-like (technically a JSON superset) text format used for the
  debugger's user and project configuration files, view rules, and metacode,
  which is parsed and used to generate code with the `metagen` layer.
- `metagen` (`MG_`): A metaprogram which is used to generate primarily code and
  data tables. Consumes Metadesk files, stored with the extension `.mdesk`, and
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
- `msf` (`MSF_`): Code for parsing and/or writing the MSF file format.
- `mule` (no namespace): Test executables for battle testing debugger
  functionality.
- `mutable_text` (`MTX_`): Implements an asynchronously-filled-and-mutated
  cache for text buffers which are mutated across time. In the debugger, this is
  used to implement the `Output` view.
- `natvis` (no namespace): NatVis files for type visualization of the codebase's
  types in other debuggers.
- `os/core` (`OS_`): An abstraction layer providing core, non-graphical
  functionality from the operating system under an abstract API, which is
  implemented per-target-operating-system.
- `os/gfx` (`OS_`): An abstraction layer, building on `os/core`, providing
  graphical operating system features under an abstract API, which is
  implemented per-target-operating-system.
- `path` (`PATH_`): Small helpers for manipulating file path strings.
- `pdb` (`PDB_`): Code for parsing and/or writing the PDB file format.
- `pe` (`PE_`): Code for parsing and/or writing the PE (Portable Executable)
  file format.
- `raddbg` (`RD_`): The layer which ties everything together for the main
  graphical debugger. Implements the debugger's graphical frontend, all of the
  debugger-specific UI, the debugger executable's command line interface, and
  all of the built-in visualizers.
- `rdi_breakpad_from_pdb` (`P2B_`): Our implementation, using the codebase's RDI
  technology, for extracting information from PDBs and generating Breakpad text
  dumps.
- `rdi_dump` (no namespace): A dumper utility program for dumping
  textualizations of RDI debug info files.
- `rdi_format` (no namespace): A layer which includes the `lib_rdi_format` layer
  and bundles it with codebase-specific helpers, to easily include the library
  in codebase programs, and have it be integrated with codebase constructs.
- `rdi_from_dwarf` (`D2R_`): Our in-progress implementation of DWARF-to-RDI
  conversion.
- `rdi_from_pdb` (`P2R_`): Our implementation of PDB-to-RDI conversion.
- `rdi_make` (no namespace): A layer which includes the `lib_rdi_make` layer and
  bundles it with codebase-specific helpers, to easily include the library in
  codebase programs, and have it be integrated with codebase constructs.
- `regs` (`REGS_`): Types, helper functions, and metadata for registers on
  supported architectures. Used in reading/writing registers in `demon`, or in
  looking up register metadata.
- `render` (`R_`): An abstraction layer providing an abstract API for rendering
  using various GPU APIs under a common interface. Does not implement a high
  level drawing API - this layer is strictly for minimally abstracting on an
  as-needed basis. Higher level drawing features are implemented in the `draw`
  layer.
- `scratch` (no namespace): Scratch space for small and transient test programs.
- `texture_cache` (`TEX_`): Implements an asynchronously-filled cache for GPU
  texture data, filled by data sourced in the `hash_store` layer's cache. Used
  for asynchronously preparing data for visualization.
- `text_cache` (`TXT_`): Implements an asynchronously-filled cache for textual
  analysis data (tokens, line ranges, and so on), filled by data sourced in the
  `hash_store` layer's cache. Used for asynchronously preparing data for
  visualization (like for the source code viewer).
- `third_party` (no namespace): External code from other projects, which some
  layers in the codebase depend on. All external code is included and built
  directly within the codebase.
- `ui` (`UI_`): Machinery for building graphical user interfaces. Provides a
  core immediate mode hierarchical user interface data structure building
  API, and has helper layers for building some higher-level widgets.
