#!/bin/bash

echo "copying spotify-android-streaming-sdk..."

# enter base directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"
base_dir="$PWD"

# enter libs directory
mkdir -p "libs" || exit $?
cd "libs" || exit $?

# copy streaming sdk lib
cp -f "../../../../external/spotify-android-streaming-sdk/spotify-player-24-noconnect-2.20b.aar" "spotify-player-24-noconnect-2.20b.aar" || exit $?
