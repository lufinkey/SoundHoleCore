#!/bin/bash

# enter base directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir/../"
base_dir="$PWD"

# clone cpp-httplib
if [ ! -e "external/cpp-httplib/.git" ]; then
        echo "fetching nodejs-embed"
        git clone --recursive "https://github.com/yhirose/cpp-httplib" "external/cpp-httplib" || ex$
fi

