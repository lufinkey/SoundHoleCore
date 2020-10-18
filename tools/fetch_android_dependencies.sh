#!/bin/bash

# enter base directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir/../"
base_dir="$PWD"

#determine which libraries should be downloaded
sqlite=false
if [ $# -eq 0 ]; then
	sqlite=true
else
	for arg; do
		if [ "$arg" == "sqlite" ]; then
			sqlite=true
		else
			>&2 echo "unknown dependency $arg"
		fi
	done
fi

# clone sqlite3
if $sqlite; then
	if [ ! -e "external/sqlite-android.aar" ]; then
		echo "fetching sqlite3"
		cd "external"
		curl "https://sqlite.org/2020/sqlite-android-3330000.aar" --output "sqlite-android.aar" || exit $?
		cd "$base_dir"
	fi
fi
