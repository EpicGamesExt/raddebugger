#!/bin/bash
set -eu
cd "$(dirname "$0")"

# compile our targets
./build.sh raddbg raddbg_non_graphical radlink radbin torture

# run torture
cd build
./torture
cd ..

