#!/bin/bash
set -eu
cd "$(dirname "$0")"

# --- Get Current Git Commit Id -----------------------------------------------
git_hash=$(git describe --always --dirty)
git_hash_full=$(git rev-parse HEAD)

# --- Compile/Link Line Definitions -------------------------------------------
cc_cflags_gcc=""
cc_cflags_clang="-fdiagnostics-absolute-paths -Wno-for-loop-analysis  -Wno-incompatible-pointer-types-discards-qualifiers -Wno-initializer-overrides -Wno-compare-distinct-pointer-types -Wno-single-bit-bitfield-constant-conversion -Wno-deprecated-declarations -Wno-writable-strings -Wno-unknown-warning-option -Wno-deprecated-register -Wno-unused-local-typedef"
cc_common="-I../src/ -I../local/ -D_GNU_SOURCE -g -DBUILD_GIT_HASH=\"$git_hash\" -DBUILD_GIT_HASH_FULL=\"$git_hash_full\" -Wall -Wno-missing-braces -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-value -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf"
cc_debug="-g -O0 -DBUILD_DEBUG=1 ${cc_common}"
cc_release="-g -O2 -DBUILD_DEBUG=0 ${cc_common}"
cc_link="-lpthread -lm -lrt -ldl"

# --- Per-Build Settings ------------------------------------------------------
cc_link_dll="-fPIC"

# --- External Libraries ------------------------------------------------------
if [[ -x "$(command -v pkg-config)" ]]; then
  cc_font_provider="$(pkg-config --cflags --libs freetype2)"  
  cc_os_gfx="$(pkg-config --cflags --libs x11 xext)"
  cc_render="$(pkg-config --cflags --libs gl egl)"
else
  cc_font_provider="-I/usr/include/freetype2 -lfreetype"
  cc_os_gfx="-lX11 -lXext"
  cc_render="-lGL -lEGL"
fi

# --- Unpack Arguments --------------------------------------------------------
for arg in "$@"; do declare $arg='1'; done
if [[ "$#" == "0" ]]; then raddbg='1'; fi

# --- Choose Compile/Link Lines -----------------------------------------------
if   [[ "${gcc:-0}"   == "1" ]]; then compiler="${CC:-gcc}   $cc_cflags_gcc";   echo "[gcc compile]";
elif [[ "${clang:-1}" == "1" ]]; then compiler="${CC:-clang} $cc_cflags_clang"; echo "[clang compile]";
fi
if   [[ "${release:-0}" == "1" ]]; then echo "[release mode]"; compile="$compiler $cc_release";
elif [[ "${debug:-1}"   == "1" ]]; then echo "[debug mode]";   compile="$compiler $cc_debug";
fi

# --- Prep Directories --------------------------------------------------------
mkdir -p build local

# --- Build & Run Metaprogram -------------------------------------------------
if [[ "${no_meta:-0}" == "0" ]]
then
  echo "[building metagen]"
  cd build
  $compiler $cc_debug ../src/metagen/metagen_main.c $cc_link -o metagen
  ./metagen
  cd ..
fi

# --- Build Everything (@build_targets) ---------------------------------------
cd build
if [[ "${raddbg:-0}"  == "1" ]]; then didbuild=1 && $compile ../src/raddbg/raddbg_main.c $cc_link $cc_os_gfx $cc_render $cc_font_provider -o raddbg; fi
if [[ "${radbin:-0}"  == "1" ]]; then didbuild=1 && $compile ../src/radbin/radbin_main.c $cc_link -o radbin; fi
if [[ "${radlink:-0}" == "1" ]]; then didbuild=1 && $compile ../src/linker/lnk.c         $cc_link -o radlink; fi
cd ..

# --- Warn On No Builds -------------------------------------------------------
if [[ "${didbuild:-0}" == "0" ]]
then
  echo "[WARNING] no valid build target specified; must use build target names as arguments to this script, like \`./build.sh raddbg\` or \`./build.sh radlink\`."
  exit 1
fi

