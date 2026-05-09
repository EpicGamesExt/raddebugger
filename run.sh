#!/bin/bash
set -eu
cd "$(dirname "$0")"

target_values=(raddbg raddbg_non_graphical radlink radbin torture)
cc_values=(clang gcc)
mode_values=(debug release)
run_torture=1
torture_args=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --)
      shift
      torture_args=("$@")
      break
      ;;
    clang|clang-*)
      cc_values=("$1")
      shift
      ;;
    gcc|gcc-*)
      cc_values=("$1")
      shift
      ;;
    debug|release)
      mode_values=("$1")
      shift
      ;;
    no_torture)
      run_torture=0
      shift
      ;;
    *)
      echo "usage: $0 [clang|clang-*|gcc|gcc-*] [debug|release] [no_torture] [-- torture-args]"
      exit 1
      ;;
  esac
done

for m in "${mode_values[@]}"; do
  for c in "${cc_values[@]}"; do
    build_cc=clang
    if [[ "$c" == gcc* ]]; then build_cc=gcc; fi

    # nuke artifacts from last run
    rm -rf build/torture_artifacts
    rm -f build/metagen
    for t in "${target_values[@]}"; do
      rm -f "build/$t"
    done

    # build targets
    CC="$c" ./build.sh meta "$build_cc" "$m" "${target_values[@]}"

    # run torture from build folder
    if [[ "$run_torture" == "1" ]]; then
      (cd build && ./torture "${torture_args[@]}")
    fi

  done
done
