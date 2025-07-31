#!/bin/bash
set -eu
cd "$(dirname "$0")"

# --- Unpack Arguments --------------------------------------------------------
gcc=0
clang=0
release=0
debug=0
no_meta=0
raddbg=0
radlink=0
rdi_from_pdb=0
rdi_from_dwarf=0
rdi_dump=0
rdi_breakpad_from_pdb=0
ryan_scratch=0
for arg in "$@"; do declare $arg='1'; done
if [ "$gcc" != "1" ];     then clang=1; fi
if [ "$release" != "1" ]; then debug=1; fi
if [ "$debug" = "1" ];     then echo "[debug mode]"; fi
if [ "$release" = "1" ];   then echo "[release mode]"; fi
if [ "$clang" = "1" ];     then compiler="${CC:-clang}"; echo "[clang compile]"; fi
if [ "$gcc" = "1" ];       then compiler="${CC:-gcc}"; echo "[gcc compile]"; fi

# --- Unpack Command Line Build Arguments -------------------------------------
auto_compile_flags=''

# --- Get Current Git Commit Id -----------------------------------------------
git_hash=$(git describe --always --dirty)
git_hash_full=$(git rev-parse HEAD)

# --- Compile/Link Line Definitions -------------------------------------------
clang_common="-I../src/ -I/usr/include/freetype2/ -I../local/ -g -DBUILD_GIT_HASH=\"$git_hash\" -DBUILD_GIT_HASH_FULL=\"$git_hash_full\" -Wno-unknown-warning-option -fdiagnostics-absolute-paths -Wall -Wno-missing-braces -Wno-unused-function -Wno-writable-strings -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-single-bit-bitfield-constant-conversion -Wno-compare-distinct-pointer-types -Wno-initializer-overrides -Wno-incompatible-pointer-types-discards-qualifiers -Wno-for-loop-analysis -Xclang -flto-visibility-public-std -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf"
clang_debug="$compiler -g -O0 -DBUILD_DEBUG=1 ${clang_common} ${auto_compile_flags}"
clang_release="$compiler -g -O2 -DBUILD_DEBUG=0 ${clang_common} ${auto_compile_flags}"
clang_link="-lpthread -lm -lrt -ldl"
clang_out="-o"
gcc_common="-I../src/ -I../local/ -g -DBUILD_GIT_HASH=\"$git_hash\" -DBUILD_GIT_HASH_FULL=\"$git_hash_full\" -Wno-unknown-warning-option -Wall -Wno-missing-braces -Wno-unused-function -Wno-attributes -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-compare-distinct-pointer-types -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf"
gcc_debug="$compiler -g -O0 -DBUILD_DEBUG=1 ${gcc_common} ${auto_compile_flags}"
gcc_release="$compiler -g -O2 -DBUILD_DEBUG=0 ${gcc_common} ${auto_compile_flags}"
gcc_link="-lpthread -lm -lrt -ldl"
gcc_out="-o"

# --- Per-Build Settings ------------------------------------------------------
link_dll="-fPIC"
link_os_gfx="-lX11 -lXext"
link_render="-lGL -lEGL"
link_font_provider="-lfreetype"

# --- Choose Compile/Link Lines -----------------------------------------------
if [ "$gcc" = "1" ];     then compile_debug="$gcc_debug"; fi
if [ "$gcc" = "1" ];     then compile_release="$gcc_release"; fi
if [ "$gcc" = "1" ];     then compile_link="$gcc_link"; fi
if [ "$gcc" = "1" ];     then out="$gcc_out"; fi
if [ "$clang" = "1" ];   then compile_debug="$clang_debug"; fi
if [ "$clang" = "1" ];   then compile_release="$clang_release"; fi
if [ "$clang" = "1" ];   then compile_link="$clang_link"; fi
if [ "$clang" = "1" ];   then out="$clang_out"; fi
if [ "$debug" = "1" ];   then compile="$compile_debug"; fi
if [ "$release" = "1" ]; then compile="$compile_release"; fi

# --- Prep Directories --------------------------------------------------------
mkdir -p build
mkdir -p local

# --- Build & Run Metaprogram -------------------------------------------------
if [ "$no_meta" = "1" ]; then echo "[skipping metagen]"; fi
if [ "$no_meta" != "1" ]
then
  cd build
  $compile_debug ../src/metagen/metagen_main.c $compile_link $out metagen
  ./metagen
  cd ..
fi

# --- Build Everything (@build_targets) ---------------------------------------
didbuild=0
cd build
if [ "$raddbg" = "1" ];                 then didbuild=1 && $compile ../src/raddbg/raddbg_main.c                                    $compile_link $link_os_gfx $link_render $link_font_provider $out raddbg; fi
if [ "$radlink" = " 1" ];               then didbuild=1 && $compile ../src/linker/lnk.c                                            $compile_link $out radlink; fi
if [ "$rdi_from_pdb" = " 1" ];          then didbuild=1 && $compile ../src/rdi_from_pdb/rdi_from_pdb_main.c                        $compile_link $out rdi_from_pdb; fi
if [ "$rdi_from_dwarf" = " 1" ];        then didbuild=1 && $compile ../src/rdi_from_dwarf/rdi_from_dwarf.c                         $compile_link $out rdi_from_dwarf; fi
if [ "$rdi_dump" = " 1" ];              then didbuild=1 && $compile ../src/rdi_dump/rdi_dump_main.c                                $compile_link $out rdi_dump; fi
if [ "$rdi_breakpad_from_pdb" = " 1" ]; then didbuild=1 && $compile ../src/rdi_breakpad_from_pdb/rdi_breakpad_from_pdb_main.c      $compile_link $out rdi_breakpad_from_pdb; fi
if [ "$ryan_scratch" = " 1" ];          then didbuild=1 && $compile ../src/scratch/ryan_scratch.c                                  $compile_link $link_os_gfx $link_render $link_font_provider $out ryan_scratch; fi
cd ..

# --- Warn On No Builds -------------------------------------------------------
if [ "$didbuild" = "0" ]
then
  echo "[WARNING] no valid build target specified; must use build target names as arguments to this script, like \`./build.sh raddbg\` or \`./build.sh rdi_from_pdb\`."
  exit 1
fi
