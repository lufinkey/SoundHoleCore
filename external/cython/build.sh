#!/bin/bash

# enter script directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"

# install cython
mkdir -p "prefix"
YTDL_PREFIX_PATH="$PWD/prefix"
pip3 install --install-option="--prefix=$YTDL_PREFIX_PATH" --ignore-installed Cython youtube-dl

# build youtube dl
PYTHONPATH="$YTDL_PREFIX_PATH" python3 setup.py build_ext --inplace
