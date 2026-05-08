#!/bin/bash
set -eu
cd "$(dirname "$0")"

target_values=(raddbg raddbg_non_graphical radlink radbin torture)
# TODO: gcc
cc_values=(clang)
mode_values=(debug release)

for m in "${mode_values[@]}"; do
  for c in "${cc_values[@]}"; do

    # nuke artifacts from last run
    rm -rf build/torture_artifacts

    ./build.sh meta "$c" "$m" "${target_values[@]}"

    # run torture
    cd build
    ./torture "$@"
    cd ..

  done
done

