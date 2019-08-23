#!/bin/bash

# enter script directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"

# install cython
mkdir -p "prefix"
YTDL_PREFIX_PATH="$PWD/prefix"
if [ -n "$(which pip3)" ]
then
	pip3 install --install-option="--prefix=$YTDL_PREFIX_PATH" Cython youtube-dl
	PYTHONPATH="$YTDL_PREFIX_PATH" python3 setup.py build_ext
else
	pip install --install-option="--prefix=$YTDL_PREFIX_PATH" Cython youtube-dl
	PYTHONPATH="$YTDL_PREFIX_PATH" python setup.py build_ext
fi
