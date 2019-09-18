# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.4.1)

file(GLOB_RECURSE SOUNDHOLE_SOURCES ../../../../../../src/soundhole/*.cpp)

add_library(
	SoundHoleCore
	STATIC
	SoundHoleCore_android.cpp
	${SOUNDHOLE_SOURCES})

find_library(
	log-lib
	log)

target_link_libraries(
	SoundHoleCore
	${log-lib})
