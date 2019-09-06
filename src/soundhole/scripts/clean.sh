#!/bin/bash

# enter script directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"
base_dir="$PWD"

# clean js folder
cd "$base_dir/js"
npm run clean

cd "$base_dir/py"
./clean.sh
