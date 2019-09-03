#!/bin/bash

# enter script directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir"
base_dir="$PWD"

# install requirements
pip3 install --prefix="$PWD/py_prefix" -r requirements.txt

# set environment variables
cd "py_prefix/lib"
python_version=(python*)
cd "$base_dir"
bundle_args=()
if [ ${#python_version[@]} -gt 0 ]; then
	python_version=${python_version[${#python_version[@]}-1]}
	modules_dir="$PWD/py_prefix/lib/$python_version/site-packages"
	bin_dir="$PWD/py_prefix/bin"
	bundle_args+=( "--add-python-path" "$modules_dir" )
	export PYTHONPATH="$modules_dir:$PYTHONPATH"
	export PATH="$bin_dir:$PATH"
fi

# bundle py file
mkdir -p "build"
stickytape "main.py" --add-python-path . "${bundle_args[@]}" --output-file "build/bundle.py"
