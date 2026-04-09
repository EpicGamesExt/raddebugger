#!/bin/bash

set -e
cd "$(dirname "$0")"

# --- Unpack Arguments --------------------------------------------------------
for arg in "$@"; do declare $arg='1'; done
if [[ ! "$gcc" ]];     then clang=1; fi
if [[ ! "$release" ]]; then debug=1; fi
if [[ $debug ]];     then echo "[debug mode]"; fi
if [[ $release ]];   then echo "[release mode]"; fi

# NOTE(yuraiz): On macOS /usr/bin/gcc is just an alias to clang anyway
compiler="${CC:-clang}"
echo "[clang compile]"

# --- Unpack Command Line Build Arguments -------------------------------------
auto_compile_flags=''

# --- Get Current Git Commit Id -----------------------------------------------
git_hash=$(git describe --always --dirty)
git_hash_full=$(git rev-parse HEAD)

# --- Compile/Link Line Definitions -------------------------------------------

# --- Per-Build Settings ------------------------------------------------------
link_render="-framework OpenGL -framework CoreFoundation -framework Cocoa"
link_font_provider="-L/opt/homebrew/lib -lfreetype"

# --- Choose Compile/Link Lines -----------------------------------------------
# NOTE(yuraiz): All warnings are disabled
clang_common="-I../src/ -I/opt/homebrew/include/freetype2/ -I../local/ -DBUILD_GIT_HASH=\"$git_hash\" -DBUILD_GIT_HASH_FULL=\"$git_hash_full\" -Wno-everything -Xclang -flto-visibility-public-std -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf"
compile_debug="$compiler -g -O0 -DBUILD_DEBUG=1 ${clang_common} ${auto_compile_flags}"
compile_release="$compiler -g -O2 -DBUILD_DEBUG=0 ${clang_common} ${auto_compile_flags}"
compile_link="-lpthread -lm -ldl"
out="-o"
if [[ $debug ]];   then compile="$compile_debug"; fi
if [[ $release ]]; then compile="$compile_release"; fi

# --- Prep Directories --------------------------------------------------------
mkdir -p build
mkdir -p local

# --- Build & Run Metaprogram -------------------------------------------------
if [[ $meta ]]
then
  echo "[doing metagen]"
  cd build
  $compile_debug ../src/metagen/metagen_main.c $compile_link $out metagen
  ./metagen
  cd ..
fi

# --- Build Everything (@build_targets) ---------------------------------------
cd build
if [[ $raddbg ]];                then didbuild=1 && $compile ../src/raddbg/raddbg_main.c                                    $compile_link $link_render $link_font_provider $out raddbg; fi
if [[ $radbin ]];                then didbuild=1 && $compile ../src/radbin/radbin_main.c                                    $compile_link $out radbin; fi
if [[ $radlink ]];               then didbuild=1 && $compile ../src/linker/lnk.c                                            $compile_link $out radlink; fi
cd ..

# --- Warn On No Builds -------------------------------------------------------
if [[ ! $didbuild ]]
then
  echo "[WARNING] no valid build target specified; must use build target names as arguments to this script, like \`./build.sh raddbg\` or \`./build.sh rdi_from_pdb\`."
  exit 1
fi
