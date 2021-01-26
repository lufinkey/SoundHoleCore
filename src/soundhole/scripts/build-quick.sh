#!/bin/bash

# enter script directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"
base_dir="$PWD"

# bundle js file
cd "$base_dir/js"
npm run build-quick || exit $?

# bundle py file
# cd "$base_dir/py"
# ./build.sh || exit $?
