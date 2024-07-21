#!/usr/bin/env bash

# --- Usage Notes (2024/1/10) ------------------------------------------------
#
# This is a central build script for the RAD Debugger project. It takes a list
# of simple alphanumeric-only arguments which control (a) what is built, (b)
# which compiler & linker are used, and (c) extra high-level build options. By
# default, if no options are passed, then the main "raddbg" graphical debugger
# is built.
#
# Below is a non-exhaustive list of possible ways to use the script:
# `build raddbg`
# `build raddbg clang`
# `build raddbg release`
# `build raddbg asan telemetry`
# `build rdi_from_pdb`
# `build gcodeview`
#
# For a full list of possible build targets and their build command lines,
# search for @build_targets in this file.
#
# Below is a list of all possible non-target command line options:
#
# - `asan`: enable address sanitizer
# - `telemetry`: enable RAD telemetry profiling support
# - `gcodeview`: generate codeview symbols instead of DRWARF for clang
# - `build_all`: don't stop on a succesful compile and build everyting

# --- Random Init -----------------------------------------------------------
# cd to script directory
self_directory="$(dirname $(readlink -f $0))"
echo "cd to '${self_directory}'"
cd "${self_directory}"
if [[ -n ${RAD_BUILD_DEBUG} ]] ; then
    shellcheck $0
fi
# RAD_META_COMPILE_ONLY

# --- Unpack Command Line Build Arguments ------------------------------------
# NOTE: With this setup you can use environment variables or arguments and
# they'll do roughly the same thing.
for x_arg in $@ ; do  declare ${x_arg}=1 ; done
if [[ -z "${msvc}" &&  -z "${clang}" ]] ; then clang=1 ; fi
if [[ -z "${release}" ]] ; then debug=1 ; fi
if [[ -n "${debug}" ]]   ; then release= && echo "[debug mode]" ; fi
if [[ -n "${release}" ]] ; then debug= && echo "[release mode]" ; fi
if [[ -n "${msvc}" ]]    ; then clang= && echo "[msvc compile]" ; fi
if [[ -n "${clang}" ]]   ; then msvc= && echo "[clang compile]" ; fi
if [[ -z "$1" ]]          ; then echo "[default mode, assuming 'raddbg' build]" && raddbg=1 ; fi
if [[ "$1" == "release" && -z "$2" ]] ; then
    echo "[default mode, assuming 'raddbg' build]" && raddbg=1 ;
fi
if [[ -n "${msvc}" ]] ; then
    clang=0
    echo "You're going to have to figure out something else if you want to use \
anything even remotely MSVC related on Linux. Bailing."
    exit 1
fi

set auto_compile_flags=
[[ -n "${telemetry}" ]] && auto_compile_flags="${auto_compile_flags} -DPROFILE_TELEMETRY=1" &&
    echo "[telemetry profiling enabled]"
[[ -n "${asan}"      ]] && auto_compile_flags="${auto_compile_flags} -fsanitize=address" &&
    "echo [asan enabled]"

# --- Compile/Link Line Definitions ------------------------------------------
    cl_common="/I../src/ /I../local/ /nologo /FC /Z7"
 clang_common="-I../src/ -I../local/ -fdiagnostics-absolute-paths -Wall -Wno-unknown-warning-option -Wno-missing-braces -Wno-unused-function -Wno-writable-strings -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-single-bit-bitfield-constant-conversion -Wno-compare-distinct-pointer-types -Xclang -flto-visibility-public-std -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf -Wl,-z,notext -lpthread -ldl -lrt"
     cl_debug="cl /Od /Ob1 /DBUILD_DEBUG=1 ${cl_common} ${auto_compile_flags}"
   cl_release="cl /O2 /DBUILD_DEBUG=0 ${cl_common} ${auto_compile_flags}"
  clang_debug="clang -g -O0 -DBUILD_DEBUG=1 ${clang_common} ${auto_compile_flags}"
clang_release="clang -g -O2 -DBUILD_DEBUG=0 ${clang_common} ${auto_compile_flags}"
      cl_link="/link /MANIFEST:EMBED /INCREMENTAL:NO \
/natvis:'${self_directory}/src/natvis/base.natvis' logo.res"
   clang_link="-fuse-ld=lld"
       cl_out="/out:"
    clang_out="-o"

# --- Per-Build Settings -----------------------------------------------------
link_dll="-DLL"
[[ -n "${msvc}"      ]] && only_compile="/c"
[[ -n "${clang}"     ]] && only_compile="-c"
[[ -n "${msvc}"      ]] && EHsc="/EHsc"
[[ -n "${clang}"     ]] && EHsc=""
[[ -n "${msvc}"      ]] && no_aslr="/DYNAMICBASE:NO"
[[ -n "${clang}"     ]] && no_aslr="-Wl,/DYNAMICBASE:NO"
[[ -n "${msvc}"      ]] && rc="rc"
[[ -n "${clang}"     ]] && rc="llvm-rc"
[[ -n "${gcodeview}" ]] && clang_common="${clang_common} -gcodeview"

# --- Choose Compile/Link Lines ----------------------------------------------
if [[ -n "${msvc}"    ]] ; then compile_debug="${cl_debug}" ; fi
if [[ -n "${msvc}"    ]] ; then compile_release="${cl_release}" ; fi
if [[ -n "${msvc}"    ]] ; then compile_link="${cl_link}" ; fi
if [[ -n "${msvc}"    ]] ; then out="${cl_out}" ; fi
if [[ -n "${clang}"   ]] ; then compile_debug="${clang_debug}" ; fi
if [[ -n "${clang}"   ]] ; then compile_release="${clang_release}" ; fi
if [[ -n "${clang}"   ]] ; then compile_link="${clang_link}" ; fi
if [[ -n "${clang}"   ]] ; then out="${clang_out}" ; fi
if [[ -n "${debug}"   ]] ; then compile="${compile_debug}" ; fi
if [[ -n "${release}" ]] ; then compile="${compile_release}" ; fi

# --- Prep Directories -------------------------------------------------------
# Si
mkdir -p "build"
mkdir -p "local"

# --- Produce Logo Icon File -------------------------------------------------
cd build
# TODO(mallchad): "Linux cannot embed icons in exes :( build a .desktop file instead
cd "${self_directory}"

# --- Get Current Git Commit Id ----------------------------------------------
# for /f ${}i in ('call git describe --always --dirty') do set compile=${compile} -DBUILD_GIT_HASH=\"${}i\"
# NOTE(mallchad): I don't really understand why this written has a loop. Is it okay without?
compile="${compile} -DBUILD_GIT_HASH=$(git describe --always --dirty)"

# --- Build & Run Metaprogram ------------------------------------------------
if [[ -n "${no_meta}" ]] ; then
    echo "[skipping metagen]"
else
  cd build
  ${compile_debug} "../src/metagen/metagen_main.c" ${compile_link} "${out}metagen.exe" || exit 1
  # Skip if compile only
  [[ -z "${RAD_META_COMPILE_ONLY}" ]] && ./metagen.exe || exit 1
  cd ${self_directory}
fi

# --- Build Everything (@build_targets) --------------------------------------

# Exit if RAD_BUILD_ALL is nonzero length
function finish()
{
    [[ -n "${build_all}" ]] && exit 1
}

# @param $1 - name of file to compile
# @param $2 - name of executable to output as
# @param $@ - rest is any arguments provided
function build_single()
{
    didbuild=1 &&
        ${compile} "$1" ${@:3:100} ${compile_link} "${out}$2" || finish
    return $?
}

function build_dll()
{
    didbuild=1 &&
        ${compile} "$1" ${compile_link} ${@:3:100} ${link_dll} "${out}$2" || finish
    return $?
}

cd build
[[ -n "${raddbg}"                ]] && build_single ../src/raddbg/raddbg_main.c                               raddbg.exe
[[ -n "${rdi_from_pdb}"          ]] && build_single ../src/rdi_from_pdb/rdi_from_pdb_main.c                   rdi_from_pdb.exe
[[ -n "${rdi_from_dwarf}"        ]] && build_single ../src/rdi_from_dwarf/rdi_from_dwarf.c                    rdi_from_dwarf.exe
[[ -n "${rdi_dump}"              ]] && build_single ../src/rdi_dump/rdi_dump_main.c                           rdi_dump.exe
[[ -n "${rdi_breakpad_from_pdb}" ]] && build_single ../src/rdi_breakpad_from_pdb/rdi_breakpad_from_pdb_main.c rdi_breakpad_from_pdb.exe
[[ -n "${ryan_scratch}"          ]] && build_single ../src/scratch/ryan_scratch.c                             ryan_scratch.exe
[[ -n "${cpp_tests}"             ]] && build_single ../src/scratch/i_hate_c_plus_plus.cpp                     cpp_tests.exe
[[ -n "${look_at_raddbg}"        ]] && build_single ../src/scratch/look_at_raddbg.c                           look_at_raddbg.exe
echo ${compile_debug}
[[ -n "${mule_main}"             ]] &&
    didbuild=1 &&
        rm -v vc*.pdb mule*.pdb &&
        ${compile_release} ${only_compile} ../src/mule/mule_inline.cpp &&
        ${compile_release} ${only_compile} ../src/mule/mule_o2.cpp &&
        ${compile_debug} ${EHsc} ../src/mule/mule_main.cpp ../src/mule/mule_c.c \
                         mule_inline.obj mule_o2.obj ${compile_link} ${no_aslr} \
                         ${out} mule_main.exe || exit 1

# Continue building the rest line normal
RAD_BUILD_ALL="1"
[[ -n "${mule_module}" ]] &&  build_dll ../src/mule/mule_module.cpp mule_module.dll || exit 1
[[ -n "${mule_hotload}" ]] && build_single ../src/mule/mule_hotload_main.c mule_hotload.exe ;
build_dll ../src/mule/mule_hotload_module_main.c mule_hotload_module.dll || exit exit 1

if [[ "${mule_peb_trample}"=="1" ]] ; then
  didbuild=1
  [[ -f "mule_peb_trample.exe"     ]] && mv "mule_peb_trample.exe" "mule_peb_trample_old_${random}.exe"
  [[ -f "mule_peb_trample_new.pdb" ]] && mv "mule_peb_trample_new.pdb" "mule_peb_trample_old_${random}.pdb"
  [[ -f "mule_peb_trample_new.rdi" ]] && mv "mule_peb_trample_new.rdi" "mule_peb_trample_old_${random}.rdi"
  build_single "../src/mule/mule_peb_trample.c" "mule_peb_trample_new.exe"
  mv "mule_peb_trample_new.exe" "mule_peb_trample.exe"
fi
cd "${self_directory}"

# --- Unset ------------------------------------------------------------------
# NOTE: Shouldn't need to unset, bash variables are volatile, even environment variables

# --- Warn On No Builds ------------------------------------------------------
if [[ -z "${didbuild}" ]] ; then
  echo "[WARNING] no valid build target specified; must use build target names as arguments to this script, like `build raddbg` or `build rdi_from_pdb`."
  exit 1
fi
