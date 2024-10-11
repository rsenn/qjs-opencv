cmake_minimum_required(VERSION 2.6)

project(LSD)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Functions.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/OpenCV.cmake)

find_opencv()

# Verbose make
#SET(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_BUILD_TYPE DEBUG)

# Library type (STATIC or SHARED)
set(LIBRARY_TYPE STATIC)

# Set link and compile flags
# -no-write-strings to remove deprecated conversion for the c-code.
# -pg for profiling the code. REMOVE WHEN DELIVERING
# -O3 -ffast-math optimization flag
set(CXX_COVERAGE_FLAGS "-Wno-write-strings -O3 -ffast-math -pg")
set(CXX_COVERAGE_LINK_FLAGS "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_COVERAGE_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_COVERAGE_LINK_FLAGS}")

#set the default path for built executables to the "bin" directory
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
#set the default path for built libraries to the "lib" directory
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

subdirs(src)
subdirs(examples)
subdirs(tests)
