#!/bin/bash

# enter base directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir/../"
base_dir="$PWD"

# determine which libraries should be downloaded
nodejs_embed=false
data_cpp=false
async_cpp=false
json11=false
if [ $# -eq 0 ]; then
	nodejs_embed=true
	data_cpp=true
	async_cpp=true
	json11=true
else
	for arg; do
		if [ "$arg" == "nodejs_embed" ]; then
			nodejs_embed=true
		elif [ "$arg" == "data_cpp" ]; then
			data_cpp=true
		elif [ "$arg" == "async_cpp" ]; then
			async_cpp=true
		elif [ "$arg" == "json11" ]; then
			json11=true
		else
			>&2 echo "unknown dependency $arg"
		fi
	done
fi

# clone nodejs-embed
if $nodejs_embed; then
	if [ ! -e "external/nodejs-embed/.git" ]; then
		echo "fetching nodejs-embed"
		git clone --recursive "git@github.com:lufinkey/nodejs-embed.git" "external/nodejs-embed" || exit $?
	fi
fi
# clone data-cpp
if $data_cpp; then
	if [ ! -e "external/data-cpp/.git" ]; then
		echo "fetching data-cpp"
		git clone --recursive "git@github.com:lufinkey/data-cpp.git" "external/data-cpp" || exit $?
	fi
fi
# clone async-cpp
if $async_cpp; then
	if [ ! -e "external/async-cpp/.git" ]; then
		echo "fetching async-cpp"
		git clone --recursive "git@github.com:lufinkey/async-cpp.git" "external/async-cpp" || exit $?
	fi
fi
# clone json11
if $json11; then
	if [ ! -e "external/json11/.git" ]; then
		echo "fetching json11"
		git clone --recursive "https://github.com/dropbox/json11.git" "external/json11" || exit $?
	fi
fi
