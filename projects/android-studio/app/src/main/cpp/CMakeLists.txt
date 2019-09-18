# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.4.1)
set( CMAKE_CXX_STANDARD 17 )

execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../tools/fetch_shared_dependencies.sh")

include_directories(
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../src"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../external/nodejs-embed/src")

add_subdirectory(
	"${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../external/nodejs-embed/projects/android-studio/libnodejsembed/src/main/cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libnodejsembed")

add_subdirectory(
	"${CMAKE_CURRENT_SOURCE_DIR}/../../../../libsoundholecore/src/main/cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../../../build/libsoundholecore")

add_library(
	native-lib
	SHARED
	native-lib.cpp)

find_library(
	log-lib
	log)

target_link_libraries( # Specifies the target library.
	native-lib

	NodeJSEmbed
	${log-lib})
