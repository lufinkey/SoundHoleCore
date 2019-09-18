#!/bin/bash

# enter base directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir/../"
base_dir="$PWD"

# clone nodejs-embed
if [ ! -e "external/nodejs-embed/.git" ]; then
	echo "fetching nodejs-embed"
	git clone --recursive "git@github.com:lufinkey/nodejs-embed.git" "external/nodejs-embed" || exit $?
fi
# clone data-cpp
if [ ! -e "external/data-cpp/.git" ]; then
	echo "fetching data-cpp"
	git clone --recursive "git@github.com:lufinkey/data-cpp.git" "external/data-cpp" || exit $?
fi
# clone async-cpp
if [ ! -e "external/async-cpp/.git" ]; then
	echo "fetching async-cpp"
	git clone --recursive "git@github.com:lufinkey/async-cpp.git" "external/async-cpp" || exit $?
fi
