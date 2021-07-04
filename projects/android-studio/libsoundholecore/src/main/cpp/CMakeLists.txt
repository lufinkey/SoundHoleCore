
project(SoundHoleCore)
cmake_minimum_required(VERSION 3.4.1)
set( CMAKE_CXX_STANDARD 17 )

set( SOUNDHOLECORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." )

execute_process(COMMAND "${SOUNDHOLECORE_ROOT}/tools/fetch_shared_dependencies.sh")
execute_process(COMMAND "${SOUNDHOLECORE_ROOT}/tools/fetch_header_dependencies.sh")
execute_process(COMMAND "${SOUNDHOLECORE_ROOT}/src/soundhole/scripts/build.sh")

file( GLOB_RECURSE SOUNDHOLE_SOURCES "${SOUNDHOLECORE_ROOT}/src/soundhole/*.cpp" )

add_library(
		SoundHoleCore
		SHARED
		_unused.cpp
		${SOUNDHOLE_SOURCES})

target_include_directories(
		SoundHoleCore
		PUBLIC
		"${SOUNDHOLECORE_ROOT}/src"
		"${SOUNDHOLECORE_ROOT}/external/nodejs-embed/src"
		"${SOUNDHOLECORE_ROOT}/external/data-cpp/src"
		"${SOUNDHOLECORE_ROOT}/external/async-cpp/src"
		"${SOUNDHOLECORE_ROOT}/external/io-cpp/src"
		"${SOUNDHOLECORE_ROOT}/external/dtl" )

target_include_directories(
		SoundHoleCore
		PRIVATE
		"${SOUNDHOLECORE_ROOT}/src/soundhole/scripts/js/build"
		"${SOUNDHOLECORE_ROOT}/external"
		"${SOUNDHOLECORE_ROOT}/external/sqlite"
		"${SOUNDHOLECORE_ROOT}/external/nodejs-embed/external/nodejs/build/mobile/include"
		"${SOUNDHOLECORE_ROOT}/external/nodejs-embed/external/nodejs/build/mobile/include/node"
		"${SOUNDHOLECORE_ROOT}/external/nodejs-embed/external/nodejs/build/mobile/node/deps/openssl/openssl/include"
		"${SOUNDHOLECORE_ROOT}/external/nodejs-embed/external/nodejs/build/addon-api" )

find_library(log-lib log)

target_link_libraries(
	SoundHoleCore
	DataCpp
	AsyncCpp
	IOCpp
	NodeJSEmbed
	${log-lib})
