
project(TestApp)
cmake_minimum_required(VERSION 3.4.1)
set( CMAKE_CXX_STANDARD 17 )

set( SOUNDHOLECORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." )

add_subdirectory(
		"${SOUNDHOLECORE_ROOT}/external/nodejs-embed/projects/android-studio/libnodejsembed/src/main/cpp"
		NodeJSEmbed_build )

add_subdirectory(
		"${SOUNDHOLECORE_ROOT}/external/data-cpp/projects/android-studio/libdatacpp/src/main/cpp"
		DataCpp_build )

add_subdirectory(
		"${SOUNDHOLECORE_ROOT}/external/async-cpp/projects/android-studio/libasynccpp/src/main/cpp"
		AsyncCpp_build )

add_subdirectory(
		"${SOUNDHOLECORE_ROOT}/external/io-cpp/projects/android-studio/libiocpp/src/main/cpp"
		IOCpp_build )

add_subdirectory(
		"${SOUNDHOLECORE_ROOT}/projects/android-studio/libsoundholecore/src/main/cpp"
		SoundHoleCore_build )

add_library(
		TestApp
		SHARED
		native-lib.cpp
		"${SOUNDHOLECORE_ROOT}/external/cxxurl/url.cpp"
		"${SOUNDHOLECORE_ROOT}/external/json11/json11.cpp"
		"${SOUNDHOLECORE_ROOT}/src/test/SoundHoleCoreTest.cpp")

target_include_directories(
		TestApp
		PRIVATE
		"${SOUNDHOLECORE_ROOT}/src"
		"${SOUNDHOLECORE_ROOT}/external/data-cpp/src"
		"${SOUNDHOLECORE_ROOT}/external/async-cpp/src"
		"${SOUNDHOLECORE_ROOT}/external/io-cpp/src"
		"${SOUNDHOLECORE_ROOT}/external")