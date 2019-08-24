#!/bin/bash

# enter script directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"

# install cython
mkdir -p "prefix"
YTDL_PREFIX_PATH="$PWD/prefix"
pip3.7 install --install-option="--prefix=$YTDL_PREFIX_PATH" --upgrade Cython youtube-dl
export PYTHONPATH="$YTDL_PREFIX_PATH/lib/python3.7/site-packages"
python3.7 setup.py build_ext
