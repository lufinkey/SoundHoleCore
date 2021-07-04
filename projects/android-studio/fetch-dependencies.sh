#!/bin/bash

# enter base directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"
base_dir="$PWD"

# fetch dependencies
cd ../../ || exit $?
./tools/fetch_shared_dependencies.sh || exit $?
./tools/fetch_header_dependencies.sh || exit $?
