project(qjs-opencv C CXX)

cmake_minimum_required(VERSION 3.5)

message("Building project [1;31m${PROJECT_NAME}[0m")

set(DISABLE_WERROR TRUE CACHE BOOL "Disable -Werror flag" FORCE)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "build type")

if(CACHE{CMAKE_BUILD_TYPE})
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel
                                               RelWithDebInfo)
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
  set(CMAKE_INSTALL_LIBDIR ""
      CACHE PATH "Specify the output directory for libraries (default is lib)")
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

check_library_exists(json-cpp ZN4YAML6detail9node_data9reset_mapEv /usr
                     JSON_CPP)

check_include_def(X11/X.h)
check_library_exists(X11 XOpenDisplay /usr/lib HAVE_LIBX11)

if(HAVE_LIBX11 AND HAVE_X11_X_H)
  set(HAVE_X11 TRUE)
endif(HAVE_LIBX11 AND HAVE_X11_X_H)

find_quickjs()
find_opencv()

#dump(OPENCV_LIBDIR OPENCV_INCLUDE_DIRS)

link_directories(${OPENCV_LIBDIR})

#set(CMAKE_INSTALL_RPATH "${OPENCV_LIB_DIR}" CACHE PATH "Build with install rpath" FORCE)
set(CMAKE_INSTALL_RPATH "${OPENCV_LIBDIR}")

include_directories(include ${OPENCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}
                    lsd/include)

include(CheckIncludeFileCXX)

set(CMAKE_REQUIRED_INCLUDES "${OPENCV_INCLUDE_DIRS}")
check_include_file_cxx(opencv2/freetype.hpp HAVE_OPENCV2_FREETYPE_HPP)
#check_library_exists(opencv_freetype _ZN2cv8freetype13FreeType2ImplC1Ev "${OPENCV_PREFIX}" OPENCV_FREETYPE)

if(HAVE_OPENCV2_FREETYPE_HPP)
  set(OPENCV_FREETYPE TRUE)
endif(HAVE_OPENCV2_FREETYPE_HPP)

if(HAVE_OPENCV2_FREETYPE_HPP AND OPENCV_FREETYPE)
  add_definitions(-DHAVE_OPENCV_FREETYPE)
  set(OPENCV_FREETYPE_LIBRARY opencv_freetype)
else(HAVE_OPENCV2_FREETYPE_HPP AND OPENCV_FREETYPE)

  dump(HAVE_OPENCV2_FREETYPE_HPP OPENCV_FREETYPE OPENCV_PREFIX)

  set(OPENCV_FREETYPE_LIBRARY)
endif(HAVE_OPENCV2_FREETYPE_HPP AND OPENCV_FREETYPE)

option(USE_FEATURE2D "Use feature2d" ON)
option(USE_BARCODE "Use barcode" ON)

if(USE_FEATURE2D)
  add_definitions(-DUSE_FEATURE2D=1)
  set(OPENCV_XFEATURES2D_LIBRARY opencv_xfeatures2d
      CACHE STRING "x features 2d library")
  set(FEATURE2D_SOURCE js_feature2d.cpp)

  check_include_file_cxx(opencv2/xfeatures2d.hpp HAVE_OPENCV2_XFEATURES2D_HPP)
  #dump(HAVE_OPENCV2_XFEATURES2D_HPP)
  if(HAVE_OPENCV2_XFEATURES2D_HPP)
    add_definitions(-DHAVE_OPENCV2_XFEATURES2D_HPP=1)
  endif(HAVE_OPENCV2_XFEATURES2D_HPP)

  # OpenCV 5.0 moved AKAZE/BRISK/KAZE/AgastFeatureDetector out of core
  # features2d into cv::xfeatures2d (contrib). AKAZE is a type name, not an
  # addressable symbol, so check_cxx_symbol_exists doesn't apply here - use
  # check_cxx_source_compiles instead.
  include(CheckCXXSourceCompiles)
  set(CMAKE_REQUIRED_INCLUDES_SAVE "${CMAKE_REQUIRED_INCLUDES}")
  set(CMAKE_REQUIRED_INCLUDES "${OPENCV_INCLUDE_DIRS}")
  check_cxx_source_compiles("
    #include <opencv2/xfeatures2d.hpp>
    int main() { cv::Ptr<cv::xfeatures2d::AKAZE> p; return 0; }
  " HAVE_OPENCV_XFEATURES2D_AKAZE)
  set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES_SAVE}")
  if(HAVE_OPENCV_XFEATURES2D_AKAZE)
    add_definitions(-DHAVE_OPENCV_XFEATURES2D_AKAZE=1)
  endif(HAVE_OPENCV_XFEATURES2D_AKAZE)
endif()
if(USE_BARCODE)
  add_definitions(-DUSE_BARCODE=1)
  set(BARCODE_SOURCE js_barcode_detector.cpp)

  check_include_file_cxx(opencv2/barcode.hpp HAVE_OPENCV2_BARCODE_HPP)
  if(HAVE_OPENCV2_BARCODE_HPP)
    add_definitions(-DHAVE_OPENCV2_BARCODE_HPP=1)
  endif(HAVE_OPENCV2_BARCODE_HPP)
endif()

check_include_cxx_def(opencv2/ximgproc.hpp HAVE_OPENCV2_XIMGPROC_HPP)
check_include_cxx_def(opencv2/ximgproc/find_ellipses.hpp
                      HAVE_OPENCV2_XIMGPROC_FIND_ELLIPSES_HPP)



#dump(HAVE_OPENCV2_XIMGPROC_HPP HAVE_OPENCV2_XIMGPROC_FIND_ELLIPSES_HPP)

#dump(HAVE_OPENCV2_XIMGPROC_HPP)
if(HAVE_OPENCV2_XIMGPROC_HPP)
  add_definitions(-DHAVE_OPENCV2_XIMGPROC_HPP=1)
  set(OPENCV_XIMGPROC_LIBRARY opencv_ximgproc)
endif(HAVE_OPENCV2_XIMGPROC_HPP)
if(HAVE_OPENCV2_XIMGPROC_FIND_ELLIPSES_HPP)
  add_definitions(-DHAVE_OPENCV2_XIMGPROC_FIND_ELLIPSES_HPP=1)
endif(HAVE_OPENCV2_XIMGPROC_FIND_ELLIPSES_HPP)

include(CheckCXXSymbolExists)
check_cxx_symbol_exists(cv::AffineFeature opencv2/features2d.hpp
                        HAVE_OPENCV_AFFINE_FEATURE)

check_include_file_cxx(opencv2/objdetect/charuco_detector.hpp HAVE_OPENCV2_CHARUCO_DETECTOR_HPP)
if(HAVE_OPENCV2_CHARUCO_DETECTOR_HPP)

endif(HAVE_OPENCV2_CHARUCO_DETECTOR_HPP)


#dump(HAVE_OPENCV2_FREETYPE_HPP OPENCV_FREETYPE_LIBRARY HAVE_OPENCV2_XFEATURES2D_HPP HAVE_OPENCV_AFFINE_FEATURE)
if(HAVE_OPENCV_AFFINE_FEATURE)
  add_definitions(-DHAVE_OPENCV_AFFINE_FEATURE)
endif(HAVE_OPENCV_AFFINE_FEATURE)

# OpenCV 5.0 moved shape/contour-analysis functions (boundingRect, moments,
# contourArea, approxPolyDP, convexHull, fitEllipse, minAreaRect, Subdiv2D,
# etc.) out of imgproc into a new "geometry" module/header.
check_include_file_cxx(opencv2/geometry.hpp HAVE_OPENCV2_GEOMETRY_HPP)
if(HAVE_OPENCV2_GEOMETRY_HPP)
  add_definitions(-DHAVE_OPENCV2_GEOMETRY_HPP=1)
endif(HAVE_OPENCV2_GEOMETRY_HPP)

# OpenCV 5.0 also moved goodFeaturesToTrack out of imgproc into a new
# "features" module/header (opencv2/features.hpp, new in 5.0).
check_include_file_cxx(opencv2/features.hpp HAVE_OPENCV2_FEATURES_HPP)
if(HAVE_OPENCV2_FEATURES_HPP)
  add_definitions(-DHAVE_OPENCV2_FEATURES_HPP=1)
endif(HAVE_OPENCV2_FEATURES_HPP)

# linearPolar/logPolar were deprecated in 4.x (in favor of warpPolar) and
# fully removed in OpenCV 5.0, with no drop-in replacement declared. Checked
# via decltype so only the declaration is needed, not a full link - some
# local OpenCV 4.x builds pull in libtbb transitively and fail to link a
# throwaway test executable even though the real (shared-library) build
# links fine.
set(CMAKE_REQUIRED_INCLUDES_SAVE "${CMAKE_REQUIRED_INCLUDES}")
set(CMAKE_REQUIRED_INCLUDES "${OPENCV_INCLUDE_DIRS}")
check_cxx_source_compiles("
  #include <opencv2/imgproc.hpp>
  int main() { decltype(&cv::linearPolar) fp = nullptr; return fp == nullptr; }
" HAVE_OPENCV_LINEARPOLAR)
check_cxx_source_compiles("
  #include <opencv2/imgproc.hpp>
  int main() { decltype(&cv::logPolar) fp = nullptr; return fp == nullptr; }
" HAVE_OPENCV_LOGPOLAR)
set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES_SAVE}")
if(HAVE_OPENCV_LINEARPOLAR)
  add_definitions(-DHAVE_OPENCV_LINEARPOLAR=1)
endif(HAVE_OPENCV_LINEARPOLAR)
if(HAVE_OPENCV_LOGPOLAR)
  add_definitions(-DHAVE_OPENCV_LOGPOLAR=1)
endif(HAVE_OPENCV_LOGPOLAR)

# OpenCV 5.0 rewrote the DNN engine: readNetFromCaffe/Darknet/Torch,
# readTorchBlob and shrinkCaffeModel were dropped, Net gained/lost several
# methods, and cv::dnn::DataLayout moved to cv::DataLayout. All landed in the
# same header revision as cv::dnn::ENGINE_AUTO, so one flag gates all of it.
# check_cxx_symbol_exists can't be used here: ENGINE_AUTO is a plain enum
# constant (not addressable), so its "&symbol" probe fails to compile even
# when the symbol exists - use check_cxx_source_compiles instead.
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES_SAVE "${CMAKE_REQUIRED_INCLUDES}")
set(CMAKE_REQUIRED_INCLUDES "${OPENCV_INCLUDE_DIRS}")
check_cxx_source_compiles("
  #include <opencv2/dnn.hpp>
  int main() { int e = cv::dnn::ENGINE_AUTO; return e - e; }
" HAVE_OPENCV_DNN_NEW_ENGINE)
set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES_SAVE}")
if(HAVE_OPENCV_DNN_NEW_ENGINE)
  add_definitions(-DHAVE_OPENCV_DNN_NEW_ENGINE)
endif(HAVE_OPENCV_DNN_NEW_ENGINE)

# OpenCV 5.0 added cv::dnn::Tokenizer (BPE tokenizer for LLM inference) and
# Net::enable/disable/resetKVCache() for autoregressive attention layers -
# both new in the same header revision as the new DNN engine, but probed
# independently since a given 5.x build's Tokenizer support isn't
# guaranteed to track ENGINE_AUTO exactly. Declaration-only (see
# linearPolar/logPolar above for why).
set(CMAKE_REQUIRED_INCLUDES_SAVE "${CMAKE_REQUIRED_INCLUDES}")
set(CMAKE_REQUIRED_INCLUDES "${OPENCV_INCLUDE_DIRS}")
check_cxx_source_compiles("
  #include <opencv2/dnn.hpp>
  using T = decltype(cv::dnn::Tokenizer::load(std::declval<std::string>()));
  int main() { decltype(&cv::dnn::Net::enableKVCache) fp = nullptr; return fp == nullptr; }
" HAVE_OPENCV_DNN_TOKENIZER)
set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES_SAVE}")
if(HAVE_OPENCV_DNN_TOKENIZER)
  add_definitions(-DHAVE_OPENCV_DNN_TOKENIZER=1)
endif(HAVE_OPENCV_DNN_TOKENIZER)

# OpenCV 5.0 replaced BarcodeDetector's (prototxt_path, model_path) ctor with
# a default ctor and a single-arg (super_resolution_model_path) ctor. Probe
# by resolving overload resolution against the old two-arg ctor via decltype
# (declaration-only - a plain symbol-exists check can't distinguish
# overloads, and actually constructing would require linking opencv_objdetect,
# which some local OpenCV 4.x builds can't do in a standalone test
# executable even though the real shared-library build links fine).
if(USE_BARCODE)
  set(CMAKE_REQUIRED_INCLUDES_SAVE "${CMAKE_REQUIRED_INCLUDES}")
  set(CMAKE_REQUIRED_INCLUDES "${OPENCV_INCLUDE_DIRS}")
  check_cxx_source_compiles("
    #include <opencv2/objdetect/barcode.hpp>
    #include <utility>
    using T = decltype(cv::barcode::BarcodeDetector(std::declval<std::string>(), std::declval<std::string>()));
    int main() { return 0; }
  " HAVE_OPENCV_BARCODE_LEGACY_CTOR)
  set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES_SAVE}")
  if(HAVE_OPENCV_BARCODE_LEGACY_CTOR)
    add_definitions(-DHAVE_OPENCV_BARCODE_LEGACY_CTOR=1)
  endif(HAVE_OPENCV_BARCODE_LEGACY_CTOR)
endif(USE_BARCODE)

# Old deprecated VideoCapture backend constants (VFW/QT/UNICAP/OpenNI-v1/
# Giganetix) and the CAP_PROP_GIGA_FRAME_*_HEIGH typo aliases were dropped in
# OpenCV 5.0. convertFp16() (deprecated in 4.x in favor of Mat::convertTo
# with CV_16F) was fully removed too.
# CAP_VFW is a plain enum constant too - same addressability problem as
# ENGINE_AUTO above, use check_cxx_source_compiles.
set(CMAKE_REQUIRED_INCLUDES_SAVE "${CMAKE_REQUIRED_INCLUDES}")
set(CMAKE_REQUIRED_INCLUDES "${OPENCV_INCLUDE_DIRS}")
check_cxx_source_compiles("
  #include <opencv2/videoio.hpp>
  int main() { int c = cv::CAP_VFW; return c - c; }
" HAVE_OPENCV_CAP_VFW)
set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES_SAVE}")
if(HAVE_OPENCV_CAP_VFW)
  add_definitions(-DHAVE_OPENCV_CAP_VFW=1)
endif(HAVE_OPENCV_CAP_VFW)
# Declaration-only check (see linearPolar/logPolar above for why).
set(CMAKE_REQUIRED_INCLUDES_SAVE "${CMAKE_REQUIRED_INCLUDES}")
set(CMAKE_REQUIRED_INCLUDES "${OPENCV_INCLUDE_DIRS}")
check_cxx_source_compiles("
  #include <opencv2/core.hpp>
  int main() { decltype(&cv::convertFp16) fp = nullptr; return fp == nullptr; }
" HAVE_OPENCV_CONVERT_FP16)
set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES_SAVE}")
if(HAVE_OPENCV_CONVERT_FP16)
  add_definitions(-DHAVE_OPENCV_CONVERT_FP16=1)
endif(HAVE_OPENCV_CONVERT_FP16)

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
if(USE_BARCODE)
  list(APPEND OPENCV_EXTRA_LIBRARIES opencv_objdetect)
endif(USE_BARCODE)
list(APPEND OPENCV_EXTRA_LIBRARIES opencv_xphoto)

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

option(QUICKJS_MODULES "Build .so modules for quickjs" ON)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

option(BUILD_QUICKJS "Build QuickJS" ON)
option(USE_LIBCAMERA "Use libcamera-opencv" OFF)
option(USE_LCCV "Use LCCV" ON)

if(ENABLE_LTO)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif(ENABLE_LTO)

link_directories(BEFORE ${quickjs_BINARY_DIR} ${OPENCV_LIBDIR})

link_directories("${CMAKE_CURRENT_BINARY_DIR}/quickjs")

set(QUICKJS_LIBRARY quickjs)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/quickjs)

if(USE_LIBCAMERA OR USE_LCCV)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/libcamera.cmake)
endif(USE_LIBCAMERA OR USE_LCCV)

file(GLOB OPENCV_SOURCES js_*.[ch]pp ${FEATURE2D_SOURCE} include/*.hpp
     src/*.cpp gifenc/gifenc.c algorithms/*.cpp lsd/src/*.cpp)

list(FILTER OPENCV_SOURCES EXCLUDE REGEX ".*_iterator.cpp")
list(FILTER OPENCV_SOURCES EXCLUDE REGEX ".*js_contour.*")

#dump(OPENCV_SOURCES)

if(USE_LCCV)
  list(APPEND OPENCV_SOURCES js_raspi_cam.cpp js_libcamera_app.cpp)
  add_definitions(-DUSE_LCCV=1)
else(USE_LCCV)
  list(REMOVE_ITEM OPENCV_SOURCES js_libcamera_app.cpp)
  list(REMOVE_ITEM OPENCV_SOURCES js_raspi_cam.cpp)
endif(USE_LCCV)

#list(REMOVE_ITEM OPENCV_SOURCES init_module.cpp)
#list(REMOVE_ITEM OPENCV_SOURCES js.hpp js.cpp)

set(CMAKE_CXX11_EXTENSION_COMPILE_OPTION "${CXX_STANDARD_FLAG}" CACHE STRING ""
                                                                      FORCE)

#if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LCCV")
if(USE_LCCV)
  set(BUILD_SHARED_LIBS OFF)
  set(LCCV_NO_INSTALL TRUE CACHE BOOL "Don't install liblccv")
  add_subdirectory(LCCV)
  include_directories(LCCV/include)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fvisibility=hidden")
  set(LCCV_LIBRARIES "${CMAKE_CURRENT_BINARY_DIR}/LCCV/liblccv.a" camera
                     camera-base event event_pthreads Threads::Threads)
  set(LCCV_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/LCCV"
      CACHE PATH "liblccv library directory")

  link_directories(${LCCV_LIBRARY_DIR})
  list(APPEND jsbindings_LINK_DIRECTORIES ${LCCV_LIBRARY_DIR})
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

list(APPEND jsbindings_LINK_DIRECTORIES ${OPENCV_LIBDIR})
list(
  APPEND
  jsbindings_LIBRARIES
  ${QUICKJS_LIBRARY}
  ${PNG_LIBRARIES}
  ${OPENCV_XIMGPROC_LIBRARY}
  ${OPENCV_XFEATURES2D_LIBRARY}
  ${OPENCV_FREETYPE_LIBRARY}
  ${OPENCV_BGSEGM_LIBRARY}
  ${OPENCV_EXTRA_LIBRARIES}
  ${OPENCV_LIBRARIES})

if(USE_LIBCAMERA OR USE_LCCV)
  list(APPEND jsbindings_LIBRARIES ${pkgcfg_lib_LIBCAMERA_camera})
endif(USE_LIBCAMERA OR USE_LCCV)

#dump(LCCV_LIBRARY_DIR)
#dump(jsbindings_LIBRARIES)

make_shared_module(opencv ${OPENCV_SOURCES})

# Force-included into every TU so that a cv::Exception escaping any binding
# becomes a catchable JS exception instead of unwinding through QuickJS's C
# frames into std::terminate(). It shadows the JS_CFUNC_*/JS_CGETSET_*/JS_CTOR_*
# macros, so it has to be seen before any function table - hence -include rather
# than a #include in each file. See include/js_exception_guard.hpp.
target_compile_options(
  quickjs-opencv PRIVATE
  "$<$<COMPILE_LANGUAGE:CXX>:-include${CMAKE_CURRENT_SOURCE_DIR}/include/js_exception_guard.hpp>")

if(USE_LCCV)
  add_dependencies(quickjs-opencv liblccv)
endif(USE_LCCV)

# make_static_module(opencv init_module.cpp)

if(OPENCV_FREETYPE_LIBRARY)
  message(STATUS "OpenCV freetype: ${OPENCV_FREETYPE_LIBRARY}")
endif(OPENCV_FREETYPE_LIBRARY)
