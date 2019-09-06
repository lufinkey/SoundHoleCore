#!/bin/bash

# enter script directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"
base_dir="$PWD"

# clone nodejs
mkdir -p "build" || exit $?
cd "build"
if [ ! -d "node/.git" ] && [ ! -f "node/.git" ]; then
	git clone --recursive "https://github.com/nodejs/node" || exit $?
fi

# get arguments
build_type="$1"
if [ -z "$build_type" ]; then
	build_type="Release"
fi
configure_args=()
if [ "$build_type" == "Debug" ]; then
	configure_args+=("--debug")
fi

# build nodejs
cd "$base_dir/build/node"
./configure --fully-static --enable-static "${configure_args[@]}" || exit $?
make -j4 || exit $?

# copy output files
cd "$base_dir/build"
mkdir -p "$build_type" || exit $?
cd "$base_dir/build/node/out/$build_type"
cp *.a "$base_dir/build/$build_type" || exit $?
