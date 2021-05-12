cmake_minimum_required(VERSION 3.0)
project(qjs-opencv)

set(DISABLE_WERROR TRUE CACHE BOOL "Disable -Werror flag" FORCE)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "build type")

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)

set(quickjs_sources_root ${CMAKE_CURRENT_SOURCE_DIR}/quickjs)

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")

if(WIN32)
  set(LIBWS2_32 ws2_32)
else(WIN32)

endif(WIN32)

include(FindThreads)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckLibraryExists)
include(CheckTypeSize)
include(FindPkgConfig)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/functions.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindQuickJS.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/opencv.cmake)

if(ENABLE_CCACHE)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ccache.cmake)
  enable_ccache()
endif(ENABLE_CCACHE)

macro(check_size TYPE VAR)
  check_type_size("${TYPE}" CMAKE_${VAR})
  if(${CMAKE_${VAR}})
    # list(APPEND PLOTCV_DEFS ${VAR}=${CMAKE_${VAR}})
  endif(${CMAKE_${VAR}})
  message(STATUS "size of ${TYPE} ${CMAKE_${VAR}}")
endmacro(check_size TYPE VAR)

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

include_directories(
  ${OPENCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/opencv
  ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/quickjs)

foreach(CXX_STANDARD c++20 c++17 c++14 c++11)
  string(REPLACE "c++" "CPLUSPLUS" CXX_STANDARD_NUM "${CXX_STANDARD}")
  set(CMAKE_REQUIRED_QUIET TRUE)
  check_cxx_compiler_flag("-std=${CXX_STANDARD}" CXX_STANDARD_${CXX_STANDARD_NUM})
  set(CMAKE_REQUIRED_QUIET FALSE)
  if(CXX_STANDARD_${CXX_STANDARD_NUM})
    string(REGEX REPLACE "\\+" "x" CXX_STANDARD_NAME "${CXX_STANDARD_VALUE}")
    string(TOUPPER "${CXX_STANDARD_NAME}" CXX_STANDARD_NAME)
    string(REGEX REPLACE "CXX" "" CXX_STANDARD_VERSION "${CXX_STANDARD_NAME}")
    message("CXX_STANDARD_NAME = ${CXX_STANDARD_NAME}")
    message("CXX_STANDARD_VERSION = ${CXX_STANDARD_VERSION}")

    if(CXX_STANDARD_NAME)
      add_definitions(-D"${CXX_STANDARD_NAME}")
    endif(CXX_STANDARD_NAME)

    set(CXX_STANDARD_VALUE "${CXX_STANDARD}" CACHE STRING "C++ standard" FORCE)
    set(CXX_STANDARD_FLAG "-std=${CXX_STANDARD}" CACHE STRING "C++ standard argument" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD_FLAG}")
    break()
  endif(CXX_STANDARD_${CXX_STANDARD_NUM})
endforeach()

string(REGEX REPLACE "c\\+\\+" "" CXX_STANDARD_VERSION "${CXX_STANDARD_VALUE}")
add_definitions(-DCXX_STANDARD=${CXX_STANDARD_VERSION})

message("C++ standard: ${CXX_STANDARD_VALUE}")

macro(NOWARN_FLAG FLAG)
  canonicalize(VARNAME "${FLAG}")
  check_c_compiler_flag("${FLAG}" "${VARNAME}")
  #dump(${VARNAME})

  if(${VARNAME})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")

  endif(${VARNAME})
endmacro(NOWARN_FLAG FLAG)

string(REGEX REPLACE " -Wall" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE " -Wall" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

nowarn_flag(-Wno-unused-value)
nowarn_flag(-Wno-unused-variable)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES ".*Clang.*")
  nowarn_flag(-Wno-deprecated-anon-enum-enum-conversion)
  nowarn_flag(-Wno-extern-c-compat)
  nowarn_flag(-Wno-implicit-int-float-conversion)
  nowarn_flag(-Wno-deprecated-enum-enum-conversion)
endif("${CMAKE_CXX_COMPILER_ID}" MATCHES ".*Clang.*")

#dump(CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
#dump(CMAKE_CXX_FLAGS_DEBUG)

check_cxx_compiler_flag("-fPIC" F_OPT_PIC)
if(F_OPT_PIC)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fPIC")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC")
endif(F_OPT_PIC)

set(DEBUG_OUTPUT TRUE CACHE BOOL "Debug output" FORCE)
set(DEBUG_MAT FALSE CACHE BOOL "Debug cv::Mat" FORCE)

check_c_compiler_flag("-O0" OPT_C_OPT_NONE)
check_cxx_compiler_flag("-O0" OPT_CXX_OPT_NONE)
if(OPT_C_OPT_NONE)
  if(NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-O0")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0" CACHE STRING "C compiler options" FORCE)
  endif(NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-O0")
endif(OPT_C_OPT_NONE)
if(OPT_CXX_OPT_NONE)
  if(NOT "${CMAKE_CXX_FLAGS_DEBUG}" MATCHES "-O0")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0" CACHE STRING "C++ compiler options"
                                                                   FORCE)
  endif(NOT "${CMAKE_CXX_FLAGS_DEBUG}" MATCHES "-O0")
endif(OPT_CXX_OPT_NONE)

check_c_compiler_flag("-ggdb" OPT_C_G_GDB)
check_cxx_compiler_flag("-ggdb" OPT_CXX_G_GDB)
if(OPT_C_G_GDB)
  if(NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-ggdb")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb" CACHE STRING "C compiler options" FORCE)
  endif(NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-ggdb")
endif(OPT_C_G_GDB)
if(OPT_CXX_G_GDB)
  if(NOT "${CMAKE_CXX_FLAGS_DEBUG}" MATCHES "-ggdb")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb" CACHE STRING "C++ compiler options"
                                                                     FORCE)
  endif(NOT "${CMAKE_CXX_FLAGS_DEBUG}" MATCHES "-ggdb")
endif(OPT_CXX_G_GDB)

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

macro(find_static_library VAR NAME)
  find_library(${VAR} NAMES lib${NAME}.a PATHS "${CMAKE_INSTALL_PREFIX}/lib" NO_DEFAULT_PATH)
endmacro()

set(QUICKJS_LIBRARY quickjs)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/quickjs)

option(QUICKJS_MODULES "Build .so modules for quickjs" ON)

if(QUICKJS_MODULES)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/js-bindings.cmake)
endif(QUICKJS_MODULES)

option(BUILD_QUICKJS "Build QuickJS" ON)

file(GLOB OPENCV_SOURCES *.hpp *.cpp)
#dump(OPENCV_SOURCES)

make_shared_module(opencv ${OPENCV_SOURCES})
# make_js_bindings()
