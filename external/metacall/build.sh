#!/bin/bash

# enter script directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"
base_dir="$PWD"

# build core
mkdir "core/build"
cd "core/build"
cmake .. -OPTION_SELF_CONTAINED=On -DOPTION_BUILD_LOADERS_C=On -DOPTION_BUILD_LOADERS_NODE=On -DOPTION_BUILD_SERIALS=On || exit $?
make || exit $?
