#!/bin/bash

if [ $# -ne 2 ]; then
	>&2 echo "invalid number of arguments. arguments are: source, destination"
	exit 1
fi

old_md5=$(cat "$1.md5")
new_md5=$(md5 "$1")

if [ -n "$old_md5" ]; then
	echo "old md5: $old_md5"
fi
if [ -n "$new_md5" ]; then
	echo "new md5: $new_md5"
fi

if [ "$old_md5" = "$new_md5" ] && [ -e "$2" ]; then
	echo "No file changes"
	exit 0
fi

echo "minifying file $1"
minify "$1" --out-file "$2" || exit $?
echo "$new_md5" > "$1.md5"
