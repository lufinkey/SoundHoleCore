#!/bin/bash

# enter script directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"
base_dir="$PWD"

# get arguments
PLATFORM="$1"
if [ -z "$PLATFORM" ]; then
	>&2 echo "platform must be given as 1st argument"
	exit 1
fi

# build core
mkdir -p "core/build/$PLATFORM"
cd "core/build/$PLATFORM"
mkdir -p "prefix"
if [ -f ".buildSuccess" ]; then
	echo "platform $platform already built. skipping..."
	exit 0
fi
cmake ../../ \
	-DCMAKE_INSTALL_PREFIX="$PWD/prefix" \
	-DOPTION_SELF_CONTAINED=On -DBUILD_SHARED_LIBS=Off \
	-DOPTION_BUILD_LOADERS_C=On -DOPTION_BUILD_LOADERS_NODE=On -DOPTION_BUILD_SERIALS=On \
	|| exit $?
make || exit $?
make install || exit $?
touch .buildSuccess
