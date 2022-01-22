#!/bin/bash

# enter base directory
base_dir=$(dirname "${BASH_SOURCE[0]}")
cd "$base_dir/../"
base_dir="$PWD"

# determine which libraries should be downloaded
nodejs_embed=false
data_cpp=false
async_cpp=false
io_cpp=false
json11=false
cxxurl=false
sqlite=false
neon=false
xml2=false
musicbrainz=false
if [ $# -eq 0 ]; then
	nodejs_embed=true
	data_cpp=true
	async_cpp=true
	io_cpp=true
	json11=true
	cxxurl=true
	sqlite=true
	neon=true
	xml2=true
	musicbrainz=true
else
	for arg; do
		if [ "$arg" == "nodejs_embed" ]; then
			nodejs_embed=true
		elif [ "$arg" == "data_cpp" ]; then
			data_cpp=true
		elif [ "$arg" == "async_cpp" ]; then
			async_cpp=true
		elif [ "$arg" == "io_cpp" ]; then
			io_cpp=true
		elif [ "$arg" == "json11" ]; then
			json11=true
		elif [ "$arg" == "cxxurl" ]; then
			cxxurl=true
		elif [ "$arg" == "sqlite" ]; then
			sqlite=true
		elif [ "$arg" == "neon" ]; then
			neon=true
		elif [ "$arg" == "xml2" ]; then
			xml2=true
		elif [ "$arg" == "musicbrainz" ]; then
			musicbrainz=true
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
# clone io-cpp
if $io_cpp; then
	if [ ! -e "external/io-cpp/.git" ]; then
		echo "fetching io-cpp"
		git clone --recursive "git@github.com:lufinkey/io-cpp.git" "external/io-cpp" || exit $?
	fi
fi
# clone json11
if $json11; then
	if [ ! -e "external/json11/.git" ]; then
		echo "fetching json11"
		git clone --recursive "https://github.com/dropbox/json11.git" "external/json11" || exit $?
	fi
fi
# clone cxxurl
if $cxxurl; then
	if [ ! -e "external/cxxurl/.git" ]; then
		echo "fetching cxxurl"
		git clone --recursive "https://github.com/chmike/CxxUrl.git" "external/cxxurl" || exit $?
	fi
fi
# clone sqlite
if $sqlite; then
	if [ ! -e "external/sqlite/sqlite3.h" ]; then
		echo "fetching sqlite3"
		cd "external"
		rm -rf "sqlite" "sqlite.zip" "sqlite-amalgamation-3360000"
		curl "https://www.sqlite.org/2021/sqlite-amalgamation-3360000.zip" --output "sqlite.zip" || exit $?
		unzip "sqlite.zip" || exit $?
		mv "sqlite-amalgamation-3360000" "sqlite"
		rm -rf "sqlite.zip"
		cd "$base_dir"
	fi
fi
# clone neon
if $neon; then
	if [ ! -e "external/neon/.git" ]; then
		echo "fetching neon"
		git clone --recursive "https://github.com/notroj/neon.git" "external/neon" || exit $?
	fi
fi
# clone xml2
if $xml2; then
	if [ ! -e "external/xml2/.git" ]; then
		echo "fetching xml2"
		git clone --recursive "https://gitlab.gnome.org/GNOME/libxml2.git" "external/xml2" || exit $?
	fi
fi
# clone musicbrainz
if $musicbrainz; then
	if [ ! -e "external/musicbrainz/.git" ]; then
		echo "fetching musicbrainz"
		git clone --recursive "https://github.com/metabrainz/libmusicbrainz.git" "external/musicbrainz" || exit $?
	fi
fi
