# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.4.1)
set( CMAKE_CXX_STANDARD 17 )

set( SOUNDHOLECORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." )

execute_process(COMMAND "${SOUNDHOLECORE_ROOT}/tools/fetch_shared_dependencies.sh")
execute_process(COMMAND "${SOUNDHOLECORE_ROOT}/tools/fetch_header_dependencies.sh")
execute_process(COMMAND "${SOUNDHOLECORE_ROOT}/src/soundhole/scripts/build.sh")

file(
		GLOB_RECURSE
		SOUNDHOLE_SOURCES
		../../../../../../src/soundhole/*.cpp
		../../../../../../src/soundhole/*.hpp
		../../../../../../src/soundhole/*.h)

add_library(
		SoundHoleCore
		STATIC
		SoundHoleCore_android.cpp
		${SOUNDHOLE_SOURCES})

target_include_directories(
		SoundHoleCore
		PUBLIC
		"${SOUNDHOLECORE_ROOT}/src")

target_include_directories(
		SoundHoleCore
		PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${SOUNDHOLECORE_ROOT}/src/soundhole/scripts/js/build"
		"${SOUNDHOLECORE_ROOT}/external/nodejs-embed/src"
		"${SOUNDHOLECORE_ROOT}/external/data-cpp/src"
		"${SOUNDHOLECORE_ROOT}/external/async-cpp/src"
		"${SOUNDHOLECORE_ROOT}/external"

		"${SOUNDHOLECORE_ROOT}/external/nodejs-embed/external/nodejs/build/mobile/include"
		"${SOUNDHOLECORE_ROOT}/external/nodejs-embed/external/nodejs/build/mobile/include/nodejs"
		"${SOUNDHOLECORE_ROOT}/external/nodejs-embed/external/nodejs/build/mobile/node/deps/openssl/openssl/include")

find_library(
	log-lib
	log)

target_link_libraries(
	SoundHoleCore
	${log-lib})
