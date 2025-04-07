#!/bin/bash
set -eu
cd "$(dirname "$0")"

# --- Unpack Arguments --------------------------------------------------------
for arg in "$@"; do declare $arg='1'; done
if [ ! -v gcc ];     then clang=1; fi
if [ ! -v release ]; then debug=1; fi
if [ -v debug ];     then echo "[debug mode]"; fi
if [ -v release ];   then echo "[release mode]"; fi
if [ -v clang ];     then compiler="${CC:-clang}"; echo "[clang compile]"; fi
if [ -v gcc ];       then compiler="${CC:-gcc}"; echo "[gcc compile]"; fi


# --- Unpack Command Line Build Arguments -------------------------------------
auto_compile_flags=''

# --- Get Current Git Commit Id -----------------------------------------------
git_hash=$(git rev-parse HEAD)
git_hash_full=$(git rev-parse HEAD)

# --- Compile/Link Line Definitions -------------------------------------------
clang_common="-I../src/ -I../local/ -I/usr/include/freetype2 -g -DBUILD_GIT_HASH=\\"$git_hash\\" -DBUILD_GIT_HASH_FULL=\\"$git_hash_full\\" -Wno-unknown-warning-option -fdiagnostics-absolute-paths -Wall -Wno-missing-braces -Wno-unused-function -Wno-writable-strings -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-single-bit-bitfield-constant-conversion -Wno-compare-distinct-pointer-types -Wno-initializer-overrides -Wno-incompatible-pointer-types-discards-qualifiers -Wno-for-loop-analysis -Xclang -flto-visibility-public-std -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf -ferror-limit=1000000"
clang_debug="$compiler -g -O0 -DBUILD_DEBUG=1 ${clang_common} ${auto_compile_flags}"
clang_release="$compiler -g -O2 -DBUILD_DEBUG=0 ${clang_common} ${auto_compile_flags}"
clang_link="-lpthread -lm -lrt -ldl -lfreetype"
clang_out="-o"
gcc_common="-I../src/ -I../local/ -I/usr/include/freetype2 -g -DBUILD_GIT_HASH=\\"$git_hash\\" -DBUILD_GIT_HASH_FULL=\\"$git_hash_full\\" -Wno-unknown-warning-option -Wall -Wno-missing-braces -Wno-unused-function -Wno-attributes -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-compare-distinct-pointer-types -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf"
gcc_debug="$compiler -g -O0 -DBUILD_DEBUG=1 ${gcc_common} ${auto_compile_flags}"
gcc_release="$compiler -g -O2 -DBUILD_DEBUG=0 ${gcc_common} ${auto_compile_flags}"
gcc_link="-lpthread -lm -lrt -ldl -lfreetype"
gcc_out="-o"

# --- Per-Build Settings ------------------------------------------------------
link_dll="-fPIC"
link_os_gfx="-lX11 -lXext -lXrandr -lGL -lGLEW"
link_agent="-lcurl"

# --- Choose Compile/Link Lines -----------------------------------------------
if [ -v gcc ];     then compile_debug="$gcc_debug -I/usr/local/include"; fi
if [ -v gcc ];     then compile_release="$gcc_release -I/usr/local/include"; fi
if [ -v gcc ];     then compile_link="$gcc_link"; fi
if [ -v gcc ];     then out="$gcc_out"; fi
if [ -v clang ];   then compile_debug="$clang_debug"; fi
if [ -v clang ];   then compile_release="$clang_release"; fi
if [ -v clang ];   then compile_link="$clang_link"; fi
if [ -v clang ];   then out="$clang_out"; fi
if [ -v debug ];   then compile="$compile_debug"; fi
if [ -v release ]; then compile="$compile_release"; fi

# --- Prep Directories --------------------------------------------------------
mkdir -p build
mkdir -p local

# --- Build & Run Metaprogram -------------------------------------------------
if [ -v no_meta ]; then echo "[skipping metagen]"; fi
if [ ! -v no_meta ]
then
  cd build
  $compile_debug ../src/metagen/metagen_main.c $compile_link $out metagen
  ./metagen
  cd ..
fi

# --- Build Everything (@build_targets) ---------------------------------------
cd build
if [ -v raddbg ];                then didbuild=1 && $compile ../src/raddbg/raddbg_main.c                                    $compile_link $link_os_gfx $out raddbg; fi
if [ -v radlink ];               then didbuild=1 && $compile ../src/linker/lnk.c                                            $compile_link $out radlink; fi
if [ -v rdi_from_pdb ];          then didbuild=1 && $compile ../src/rdi_from_pdb/rdi_from_pdb_main.c                        $compile_link $out rdi_from_pdb; fi
if [ -v rdi_from_dwarf ];        then didbuild=1 && $compile ../src/rdi_from_dwarf/rdi_from_dwarf.c                         $compile_link $out rdi_from_dwarf; fi
if [ -v rdi_dump ];              then didbuild=1 && $compile ../src/rdi_dump/rdi_dump_main.c                                $compile_link $out rdi_dump; fi
if [ -v rdi_breakpad_from_pdb ]; then didbuild=1 && $compile ../src/rdi_breakpad_from_pdb/rdi_breakpad_from_pdb_main.c      $compile_link $out rdi_breakpad_from_pdb; fi
if [ -v ryan_scratch ];          then didbuild=1 && $compile ../src/scratch/ryan_scratch.c                                  $compile_link $link_os_gfx $out ryan_scratch; fi
if [ -v whatsapp ];              then didbuild=1 && $compile ../src/whatsapp/whatsapp.c                                    $compile_link $out whatsapp; fi
if [ -v agent ];                 then didbuild=1 && $compile ../src/agent/agent.c                                          $compile_link $link_agent $out agent; fi
if [ -v browser ];               then didbuild=1 && $compile ../src/browser/browser.c                                       $compile_link $out browser; fi
if [ -v cad ];                   then didbuild=1 && $compile ../src/cad/cad.c                                              $compile_link $out cad; fi
if [ -v daw ];                   then didbuild=1 && $compile ../src/daw/daw.c                                              $compile_link $out daw; fi
if [ -v editor ];                then didbuild=1 && $compile ../src/editor/editor.c                                         $compile_link $out editor; fi
if [ -v mail ];                  then didbuild=1 && $compile ../src/mail/mail.c                                            $compile_link $out mail; fi
if [ -v paint ];                 then didbuild=1 && $compile ../src/paint/paint.c                                          $compile_link $out paint; fi
if [ -v stockitup ];             then didbuild=1 && $compile ../src/stockitup/stockitup.c                                  $compile_link $out stockitup; fi
if [ -v terminal ];              then didbuild=1 && $compile ../src/terminal/terminal.c                                     $compile_link $out terminal; fi
cd ..

# --- Warn On No Builds -------------------------------------------------------
if [ ! -v didbuild ]
then
  echo "[WARNING] no valid build target specified; must use build target names as arguments to this script, like \`./build.sh raddbg\` or \`./build.sh rdi_from_pdb\`."
  exit 1
fi
