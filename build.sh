#!/bin/bash
cd "$(dirname "$0")"

# --- Unpack Arguments --------------------------------------------------------
for arg in "$@"; do declare $arg='1'; done
if [ ! "$gcc" = "1" ];     then clang=1; fi
if [ ! "$release" = "1" ]; then debug=1; fi
if [ "$debug" = "1" ];   then release=0 && echo "[debug mode]"; fi
if [ "$release" = "1" ]; then debug=0   && echo "[release mode]"; fi
if [ "$clang" = "1" ];   then gcc=0     && echo "[clang compile]"; fi
if [ "$gcc" = "1" ];     then clang=0   && echo "[gcc compile]"; fi

# --- Unpack Command Line Build Arguments -------------------------------------
auto_compile_flags=''

# --- Compile/Link Line Definitions -------------------------------------------
clang_common='-I../src/ -I../local/ -gcodeview -fdiagnostics-absolute-paths -Wall -Wno-unknown-warning-option -Wno-missing-braces -Wno-unused-function -Wno-writable-strings -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-single-bit-bitfield-constant-conversion -Wno-compare-distinct-pointer-types -Wno-initializer-overrides -Wno-incompatible-pointer-types-discards-qualifiers -Xclang -flto-visibility-public-std -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf'
clang_debug="clang -g -O0 -DBUILD_DEBUG=1 ${clang_common} ${auto_compile_flags}"
clang_release="clang -g -O2 -DBUILD_DEBUG=0 ${clang_common} ${auto_compile_flags}"
clang_link="-lpthread"
clang_out="-o"

# --- Per-Build Settings ------------------------------------------------------
link_dll="-fPIC"

# --- Choose Compile/Link Lines -----------------------------------------------
if [ "$clang" = "1" ];   then compile_debug="$clang_debug"; fi
if [ "$clang" = "1" ];   then compile_release="$clang_release"; fi
if [ "$clang" = "1" ];   then compile_link="$clang_link"; fi
if [ "$clang" = "1" ];   then out="$clang_out"; fi
if [ "$debug" = "1" ];   then compile="$compile_debug"; fi
if [ "$release" = "1" ]; then compile="$compile_release"; fi

# --- Prep Directories --------------------------------------------------------
if [ ! -d build ]; then mkdir build; fi
if [ ! -d local ]; then mkdir local; fi

# --- Build & Run Metaprogram -------------------------------------------------
if [ "$no_meta" = "1" ]; then echo "[skipping metagen]"; fi
if [ "$no_meta" = "" ]
then
  cd build
  $compile_debug ../src/metagen/metagen_main.c $compile_link $out metagen || exit 1
  ./metagen || exit 1
  cd ..
fi

# --- Build Everything (@build_targets) ---------------------------------------
cd build
if [ "$raddbg" = "1" ];        then didbuild=1 && $compile ../src/raddbg/raddbg_main.c     $compile_link $out raddbg || exit 1; fi
if [ "$ryan_scratch" = "1" ];  then didbuild=1 && $compile ../src/scratch/ryan_scratch.c   $compile_link $out ryan_scratch || exit 1; fi
cd ..

# --- Warn On No Builds -------------------------------------------------------
if [ "$didbuild" = "" ]
then
  echo "[WARNING] no valid build target specified; must use build target names as arguments to this script, like \`./build.sh raddbg\` or \`./build.sh rdi_from_pdb\`."
  exit 1
fi
