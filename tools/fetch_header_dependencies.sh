#!/bin/bash

# enter base directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir/../"
base_dir="$PWD"

# determine which libraries should be downloaded
httplib=false
if [ $# -eq 0 ]; then
	httplib=true
else
	for arg; do
		if [ "$arg" == "httplib" ]; then
			httplib=true
		else
			>&2 echo "unknown dependency $arg"
		fi
	done
fi

# clone httplib
if $httplib; then
	if [ ! -e "external/cpp-httplib/.git" ]; then
		echo "fetching nodejs-embed"
		git clone --recursive "https://github.com/yhirose/cpp-httplib" "external/httplib" || ex$
	fi
fi
