cmake_minimum_required(VERSION 3.0)
project(qjs-opencv)

set(DISABLE_WERROR TRUE CACHE BOOL "Disable -Werror flag" FORCE)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "build type")

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)

set(QUICKJS_SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/quickjs)

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")

if(WIN32)
  set(LIBWS2_32 ws2_32)
else(WIN32)

endif(WIN32)

include(FindThreads)
#include(FindPNG)
include(FindPkgConfig)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Functions.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckFlags.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindQuickJS.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/JSBindings.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenCV.cmake)

pkg_search_module(PNG libpng16)
#pkg_get_variable(PNG_INCLUDE_DIR "${PNG_MODULE_NAME}" includedir)
dump(PNG_MODULE_NAME PNG_INCLUDE_DIRS)

if(ENABLE_CCACHE)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CCache.cmake)
  enable_ccache()
endif(ENABLE_CCACHE)

check_size("long" SIZEOF_LONG)
check_size("long long" SIZEOF_LONG_LONG)
check_size("size_t" SIZEOF_SIZE_T)

if(NOT POSITION_INDEPENDENT_CODE)
  set(POSITION_INDEPENDENT_CODE ON)
endif(NOT POSITION_INDEPENDENT_CODE)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

message("Thread library: ${CMAKE_THREAD_LIBS_INIT}")
if(CMAKE_THREAD_LIBS_INIT)
  set(LIBPTHREAD ${CMAKE_THREAD_LIBS_INIT})
endif(CMAKE_THREAD_LIBS_INIT)

check_library_exists(m atan2 /usr/lib HAVE_LIBM)
if(HAVE_LIBM)
  set(LIBM m)
  # message("Found math library: ${LIBM}")
endif(HAVE_LIBM)

check_library_exists(dl dlopen /usr/lib HAVE_LIBDL)
if(HAVE_LIBDL)
  set(LIBDL dl)
  # message("Found dlfcn library: ${LIBDL}")
endif(HAVE_LIBDL)

check_library_exists(json-cpp ZN4YAML6detail9node_data9reset_mapEv /usr JSON_CPP)

find_quickjs()
find_opencv()

set(CMAKE_INSTALL_RPATH "${OPENCV_LIB_DIR}" CACHE PATH "Build with install rpath" FORCE)

include_directories(${OPENCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

include(CheckIncludeFileCXX)

set(CMAKE_REQUIRED_INCLUDES "${OPENCV_INCLUDE_DIRS}")
check_include_file_cxx(opencv2/freetype.hpp HAVE_OPENCV2_FREETYPE_HPP)
if(HAVE_OPENCV2_FREETYPE_HPP)
  add_definitions(-DHAVE_OPENCV_FREETYPE)
  set(OPENCV_FREETYPE_LIBRARY opencv_freetype)
endif(HAVE_OPENCV2_FREETYPE_HPP)

check_include_file_cxx(opencv2/xfeatures2d.hpp HAVE_OPENCV2_XFEATURES2D_HPP)
if(HAVE_OPENCV2_XFEATURES2D_HPP)
  add_definitions(-DHAVE_OPENCV2_XFEATURES2D_HPP)
endif(HAVE_OPENCV2_XFEATURES2D_HPP)

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(cv::AffineFeature opencv2/features2d.hpp HAVE_OPENCV_AFFINE_FEATURE)

dump(HAVE_OPENCV2_FREETYPE_HPP OPENCV_FREETYPE_LIBRARY HAVE_OPENCV2_XFEATURES2D_HPP HAVE_OPENCV_AFFINE_FEATURE)
if(HAVE_OPENCV_AFFINE_FEATURE)
  add_definitions(-DHAVE_OPENCV_AFFINE_FEATURE)
endif(HAVE_OPENCV_AFFINE_FEATURE)

check_cxx_standard_flag()

string(REGEX REPLACE " -Wall" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE " -Wall" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

check_nowarn_flag(-Wno-unused-value)
check_nowarn_flag(-Wno-unused-variable)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES ".*Clang.*")
  check_nowarn_flag(-Wno-deprecated-anon-enum-enum-conversion)
  check_nowarn_flag(-Wno-extern-c-compat)
  check_nowarn_flag(-Wno-implicit-int-float-conversion)
  check_nowarn_flag(-Wno-deprecated-enum-enum-conversion)
endif("${CMAKE_CXX_COMPILER_ID}" MATCHES ".*Clang.*")

# dump(CMAKE_C_FLAGS CMAKE_CXX_FLAGS) dump(CMAKE_CXX_FLAGS_DEBUG)

check_pic_flag(QUICKJS_MODULE_CFLAGS)

set(DEBUG_OUTPUT TRUE CACHE BOOL "Debug output" FORCE)
set(DEBUG_MAT FALSE CACHE BOOL "Debug cv::Mat" FORCE)

check_opt_none_flag()

check_debug_gdb_flag()

check_c_compiler_flag("-flto" HAS_F_LTO_FLAG)

if(HAS_F_LTO_FLAG)
  option(ENABLE_LTO "Enable link-time optimization" OFF)
endif(HAS_F_LTO_FLAG)

if(ENABLE_LTO)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif(ENABLE_LTO)

link_directories(BEFORE ${quickjs_BINARY_DIR})

link_directories("${CMAKE_CURRENT_BINARY_DIR}/quickjs")

set(QUICKJS_LIBRARY quickjs)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/quickjs)

option(QUICKJS_MODULES "Build .so modules for quickjs" ON)

option(BUILD_QUICKJS "Build QuickJS" ON)

file(GLOB OPENCV_SOURCES *.hpp *.cpp)
# dump(OPENCV_SOURCES)

dump(QUICKJS_MODULE_CFLAGS F_PIC)

list(REMOVE_ITEM OPENCV_SOURCES init_module.cpp)
list(REMOVE_ITEM OPENCV_SOURCES js.hpp js.cpp)

set(CMAKE_CXX11_EXTENSION_COMPILE_OPTION "${CXX_STANDARD_FLAG}" CACHE STRING "" FORCE)

add_library(qjs-opencv STATIC ${OPENCV_SOURCES})
set_target_properties(qjs-opencv PROPERTIES PREFIX "" OUTPUT_NAME opencv COMPILE_FLAGS "${QUICKJS_MODULE_CFLAGS} ${F_PIC}")
target_compile_definitions(qjs-opencv PRIVATE CONFIG_PREFIX="${CMAKE_INSTALL_PREFIX}")
dump(PNG_INCLUDE_DIRS PNG_LIBRARIES PNG_LIBRARY OPENCV_FREETYPE_LIBRARY)
target_include_directories(qjs-opencv PUBLIC "${PNG_INCLUDE_DIRS}")
target_link_libraries(qjs-opencv PUBLIC ${PNG_LIBRARIES} ${OPENCV_FREETYPE_LIBRARY})

install(TARGETS qjs-opencv DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/quickjs)
set(jsbindings_LIBRARIES qjs-opencv)

# make_static_module(opencv init_module.cpp)
make_shared_module(opencv init_module.cpp)

if(OPENCV_FREETYPE_LIBRARY)
  message(STATUS "OpenCV freetype: ${OPENCV_FREETYPE_LIBRARY}")
endif(OPENCV_FREETYPE_LIBRARY)
