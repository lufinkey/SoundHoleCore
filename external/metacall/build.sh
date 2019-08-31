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
BUILD_TYPE="$2"
if [ -z "$BUILD_TYPE" ]; then
	BUILD_TYPE="Release"
fi

# build core
mkdir -p "core/build/$PLATFORM"
cd "core/build/$PLATFORM"
mkdir -p "prefix"
if [ -f ".buildSuccess" ]; then
	echo "platform $platform already built. skipping..."
	exit 0
fi
echo ""; echo ""; echo 'running cmake'; echo ""; echo ""
cmake ../../ \
	-DCMAKE_INSTALL_PREFIX="$PWD/prefix" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
	-DOPTION_SELF_CONTAINED=On -DOPTION_FORK_SAFE=On -DOPTION_BUILD_DOCS=On \
	-DOPTION_BUILD_LOADERS_C=On -DOPTION_BUILD_LOADERS_NODE=On -DOPTION_BUILD_LOADERS_PY=Off \
	-DOPTION_BUILD_LOADERS_JS=Off -DOPTION_BUILD_LOADERS_CS=Off -DOPTION_BUILD_LOADERS_RB=Off \
	-DOPTION_BUILD_SERIALS=On \
	|| exit $?
echo ""; echo ""; echo 'running make'; echo ""; echo ""
make || exit $?
echo ""; echo ""; echo 'running make install'; echo ""; echo ""
make install || exit $?
touch .buildSuccess
