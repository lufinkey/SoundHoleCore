# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.4.1)
set( CMAKE_CXX_STANDARD 17 )

set( SOUNDHOLECORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." )

add_subdirectory(
		"${SOUNDHOLECORE_ROOT}/external/nodejs-embed/projects/android-studio/libnodejsembed/src/main/cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libnodejsembed")

add_subdirectory(
		"${SOUNDHOLECORE_ROOT}/external/data-cpp/projects/android-studio/libdatacpp/src/main/cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libdatacpp")

add_subdirectory(
		"${SOUNDHOLECORE_ROOT}/external/async-cpp/projects/android-studio/libasynccpp/src/main/cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libasynccpp")

add_subdirectory(
		"${CMAKE_CURRENT_SOURCE_DIR}/../../../../libsoundholecore/src/main/cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libsoundholecore")

add_library(
		native-lib
		SHARED
		native-lib.cpp
		"${SOUNDHOLECORE_ROOT}/external/cxxurl/url.cpp"
		"${SOUNDHOLECORE_ROOT}/external/json11/json11.cpp"
		"${SOUNDHOLECORE_ROOT}/src/test/SoundHoleCoreTest.cpp")

target_include_directories(
		native-lib
		PRIVATE
		"${SOUNDHOLECORE_ROOT}/src"
		"${SOUNDHOLECORE_ROOT}/external/data-cpp/src"
		"${SOUNDHOLECORE_ROOT}/external/async-cpp/src"
		"${SOUNDHOLECORE_ROOT}/external")

find_library(
		log-lib
		log)

target_link_libraries( # Specifies the target library.
		native-lib

		NodeJSEmbed
		DataCpp
		AsyncCpp
		SoundHoleCore
		${log-lib})
