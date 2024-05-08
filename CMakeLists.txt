cmake_minimum_required(VERSION 3.0)
project(qjs-opencv)

message("Building project [1;31m${PROJECT_NAME}[0m")

set(DISABLE_WERROR TRUE CACHE BOOL "Disable -Werror flag" FORCE)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "build type")

if(CACHE{CMAKE_BUILD_TYPE})
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)
endif(CACHE{CMAKE_BUILD_TYPE})

set(QUICKJS_SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/quickjs)

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")

if(WIN32)
  set(LIBWS2_32 ws2_32)
else(WIN32)

endif(WIN32)

include(FindThreads)
if(NOT PKG_CONFIG_FOUND)
  include(FindPkgConfig)
endif(NOT PKG_CONFIG_FOUND)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/MoreFunctions.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckFlags.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/UseMultiArch.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindQuickJS.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindOpenCV.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/QuickJSModule.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/JSBindings.cmake)

if(UNIX AND NOT APPLE)
  include(GNUInstallDirs)
elseif(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR "" CACHE PATH
                                    "Specify the output directory for libraries (default is lib)")
endif()

message(STATUS "Finding QuickJS...")

find_quickjs()
configure_quickjs()

pkg_search_module(PNG libpng16)
#pkg_get_variable(PNG_INCLUDE_DIR "${PNG_MODULE_NAME}" includedir)
#dump(PNG_MODULE_NAME PNG_INCLUDE_DIRS)

if(ENABLE_CCACHE)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CCache.cmake)
  enable_ccache()
endif(ENABLE_CCACHE)

check_size("long" SIZEOF_LONG)
check_size("long long" SIZEOF_LONG_LONG)
check_size("size_t" SIZEOF_SIZE_T)

include(CheckFunctionExists)

check_function_def(strerror_s)
if(HAVE_STRERROR_S)
  add_definitions(-D__STDC_LIB_EXT1__)
endif(HAVE_STRERROR_S)

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

check_include_def(X11/X.h)
check_library_exists(X11 XOpenDisplay /usr/lib HAVE_LIBX11)

if(HAVE_LIBX11 AND HAVE_X11_X_H)
  set(HAVE_X11 TRUE)
endif(HAVE_LIBX11 AND HAVE_X11_X_H)

find_quickjs()
find_opencv()

#set(CMAKE_INSTALL_RPATH "${OPENCV_LIB_DIR}" CACHE PATH "Build with install rpath" FORCE)
set(CMAKE_INSTALL_RPATH "${OPENCV_LIBDIR}")

include_directories(${OPENCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} lsd/include)

include(CheckIncludeFileCXX)

set(CMAKE_REQUIRED_INCLUDES "${OPENCV_INCLUDE_DIRS}")
check_include_file_cxx(opencv2/freetype.hpp HAVE_OPENCV2_FREETYPE_HPP)
check_library_exists(opencv_freetype _ZN2cv8freetype13FreeType2ImplC1Ev /usr OPENCV_FREETYPE)
if(HAVE_OPENCV2_FREETYPE_HPP AND OPENCV_FREETYPE)
  add_definitions(-DHAVE_OPENCV_FREETYPE)
  set(OPENCV_FREETYPE_LIBRARY opencv_freetype)
else(HAVE_OPENCV2_FREETYPE_HPP AND OPENCV_FREETYPE)
  set(OPENCV_FREETYPE_LIBRARY)
endif(HAVE_OPENCV2_FREETYPE_HPP AND OPENCV_FREETYPE)

check_include_file_cxx(opencv2/xfeatures2d.hpp HAVE_OPENCV2_XFEATURES2D_HPP)
#dump(HAVE_OPENCV2_XFEATURES2D_HPP)
if(HAVE_OPENCV2_XFEATURES2D_HPP)
  add_definitions(-DHAVE_OPENCV2_XFEATURES2D_HPP=1)
  set(OPENCV_XFEATURES2D_LIBRARY opencv_xfeatures2d)
  set(FEATURE2D_SOURCE js_feature2d.cpp)
endif(HAVE_OPENCV2_XFEATURES2D_HPP)

check_include_file_cxx(opencv2/ximgproc.hpp HAVE_OPENCV2_XIMGPROC_HPP)
#dump(HAVE_OPENCV2_XIMGPROC_HPP)
if(HAVE_OPENCV2_XIMGPROC_HPP)
  add_definitions(-DHAVE_OPENCV2_XIMGPROC_HPP=1)
  set(OPENCV_XIMGPROC_LIBRARY opencv_ximgproc)
endif(HAVE_OPENCV2_XIMGPROC_HPP)

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(cv::AffineFeature opencv2/features2d.hpp HAVE_OPENCV_AFFINE_FEATURE)

#dump(HAVE_OPENCV2_FREETYPE_HPP OPENCV_FREETYPE_LIBRARY HAVE_OPENCV2_XFEATURES2D_HPP HAVE_OPENCV_AFFINE_FEATURE)
if(HAVE_OPENCV_AFFINE_FEATURE)
  add_definitions(-DHAVE_OPENCV_AFFINE_FEATURE)
endif(HAVE_OPENCV_AFFINE_FEATURE)

#[[check_include_file_cxx(opencv2/bgsegm.hpp HAVE_OPENCV2_BGSEGM_HPP)
check_library_exists(opencv_bgsegm _ZN2cv6bgsegm30createBackgroundSubtractorLSBPEiiiffffffffii /usr
                     OPENCV_BGSEGM)
if(HAVE_OPENCV2_BGSEGM_HPP AND OPENCV_BGSEGM)
  add_definitions(-DHAVE_OPENCV_BGSEGM)
  set(OPENCV_BGSEGM_LIBRARY opencv_bgsegm)
else(HAVE_OPENCV2_BGSEGM_HPP AND OPENCV_BGSEGM)
  set(OPENCV_BGSEGM_LIBRARY)
endif(HAVE_OPENCV2_BGSEGM_HPP AND OPENCV_BGSEGM)]]
set(OPENCV_BGSEGM_LIBRARY opencv_bgsegm)

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

#dump(CMAKE_C_FLAGS CMAKE_CXX_FLAGS) dump(CMAKE_CXX_FLAGS_DEBUG)

check_pic_flag(QUICKJS_MODULE_CFLAGS)

#set(DEBUG_OUTPUT TRUE CACHE BOOL "Debug output" FORCE)
#set(DEBUG_MAT FALSE CACHE BOOL "Debug cv::Mat" FORCE)

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
option(USE_LIBCAMERA "Use libcamera-opencv" OFF)
option(USE_LCCV "Use LCCV" ON)

if(USE_LIBCAMERA OR USE_LCCV)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/libcamera.cmake)
endif(USE_LIBCAMERA OR USE_LCCV)

file(
  GLOB
  OPENCV_SOURCES
  init_module.cpp
  geometry.cpp
  highgui.cpp
  jsbindings.cpp
  js_clahe.cpp
  js_contour.cpp
  js_cv.cpp
  js_draw.cpp
  js_fast_line_detector.cpp
  ${FEATURE2D_SOURCE}
  js_imgproc.cpp
  js_keypoint.cpp
  js_line.cpp
  js_line_iterator.cpp
  js_line_segment_detector.cpp
  js_mat.cpp
  js_affine3.cpp
  js_point.cpp
  js_point_iterator.cpp
  js_rect.cpp
  js_rotated_rect.cpp
  js_size.cpp
  js_slice_iterator.cpp
  js_subdiv2d.cpp
  js_umat.cpp
  js_utility.cpp
  js_bg_subtractor.cpp
  js_white_balancer.cpp
  js_video_capture.cpp
  js_video_writer.cpp
  line.cpp
  util.cpp
  gifenc/gifenc.c
  dominant_colors_grabber.cpp
  lsd/src/*.cpp)
#dump(OPENCV_SOURCES)

if(USE_LCCV)
  list(APPEND OPENCV_SOURCES js_raspi_cam.cpp js_libcamera_app.cpp)
  add_definitions(-DUSE_LCCV=1)
endif(USE_LCCV)

list(REMOVE_ITEM OPENCV_SOURCES init_module.cpp)
list(REMOVE_ITEM OPENCV_SOURCES js.hpp js.cpp)

set(CMAKE_CXX11_EXTENSION_COMPILE_OPTION "${CXX_STANDARD_FLAG}" CACHE STRING "" FORCE)

#if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LCCV")
if(USE_LCCV)
  set(BUILD_SHARED_LIBS OFF)
  set(LCCV_NO_INSTALL TRUE CACHE BOOL "Don't install liblccv")
  add_subdirectory(LCCV)
  include_directories(LCCV/include)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fvisibility=hidden")
  set(LCCV_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/LCCV/liblccv.a" camera camera-base event
                     event_pthreads Threads::Threads)
  set(LCCV_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/LCCV" CACHE PATH "liblccv library directory")

  link_directories(${LCCV_LIBRARY_DIR})
  set(jsbindings_LINK_DIRECTORIES ${LCCV_LIBRARY_DIR})
  list(APPEND jsbindings_LIBRARIES lccv)
endif(USE_LCCV)
#endif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LCCV")

#add_library(qjs-opencv STATIC ${OPENCV_SOURCES})
#target_compile_definitions(qjs-opencv PRIVATE CONFIG_PREFIX="${CMAKE_INSTALL_PREFIX}")
#target_include_directories(qjs-opencv PUBLIC ${PNG_INCLUDE_DIRS} ${LIBCAMERA_INCLUDE_DIRS})
#if(USE_LCCV)
#  add_dependencies(qjs-opencv liblccv)
#endif(USE_LCCV)
#
#make_module(opencv init_module.cpp)

list(APPEND jsbindings_LIBRARIES ${QUICKJS_LIBRARY} ${PNG_LIBRARIES} ${OPENCV_XIMGPROC_LIBRARY}
     ${OPENCV_XFEATURES2D_LIBRARY} ${OPENCV_FREETYPE_LIBRARY} ${OPENCV_BGSEGM_LIBRARY}
     ${OPENCV_LIBRARIES})

if(USE_LIBCAMERA OR USE_LCCV)
  list(APPEND jsbindings_LIBRARIES ${pkgcfg_lib_LIBCAMERA_camera})
endif(USE_LIBCAMERA OR USE_LCCV)

#dump(LCCV_LIBRARY_DIR)
#dump(jsbindings_LIBRARIES)

make_shared_module(opencv ${OPENCV_SOURCES})

if(USE_LCCV)
  add_dependencies(quickjs-opencv liblccv)
endif(USE_LCCV)

# make_static_module(opencv init_module.cpp)

if(OPENCV_FREETYPE_LIBRARY)
  message(STATUS "OpenCV freetype: ${OPENCV_FREETYPE_LIBRARY}")
endif(OPENCV_FREETYPE_LIBRARY)
