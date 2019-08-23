#!/bin/bash

# enter script directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"

# delete outputted files
rm -rf build prefix *.so *.o lib/*.so lib/*.o lib/*.h lib/*.cpp
