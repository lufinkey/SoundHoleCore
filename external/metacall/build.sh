#!/bin/bash

# enter script directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"
base_dir="$PWD"

# build core
mkdir "core/build"
cd "core/build"
cmake .. || exit $?
make || exit $?
