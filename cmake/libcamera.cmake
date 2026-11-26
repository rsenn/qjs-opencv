add_definitions(-D_FILE_OFFSET_BITS=64)
if(CMAKE_COMPILER_IS_GNUCXX)
  add_compile_options(-Wno-psabi)
endif()

# no CMAKE_CXX_STANDARD here: it appends -std=gnu++17 after the project's own -std=c++2a

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindLibcamera.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildLibcamera.cmake)

# normalize_tristate(OUTVAR VALUE): ON/TRUE/YES/1 -> ON, OFF/FALSE/NO/0/"" -> OFF,
# AUTO -> AUTO.
macro(libcamera_tristate OUTVAR VALUE)
  string(TOUPPER "${VALUE}" _lc_v)
  if(_lc_v STREQUAL "AUTO")
    set(${OUTVAR} AUTO)
  elseif(_lc_v MATCHES "^(ON|TRUE|YES|Y|1)$")
    set(${OUTVAR} ON)
  elseif(_lc_v MATCHES "^(OFF|FALSE|NO|N|0|)$|-NOTFOUND$")
    set(${OUTVAR} OFF)
  else()
    message(FATAL_ERROR "${OUTVAR}: '${VALUE}' is not ON, AUTO or OFF")
  endif()
  unset(_lc_v)
endmacro(libcamera_tristate)

libcamera_tristate(_lc_want_lccv "${USE_LCCV}")
libcamera_tristate(_lc_build "${BUILD_LIBCAMERA}")

# An explicit request is one given as ON; AUTO only asks for the feature when
# libcamera is there. The cache keeps what the user wrote; the normal
# variable USE_LCCV below is always plain ON/OFF, so every if(USE_LCCV) in
# the project keeps working (AUTO would count as true).
set(_lc_explicit FALSE)
if(_lc_want_lccv STREQUAL "ON")
  set(_lc_explicit TRUE)
endif()

set(LIBCAMERA_FOUND FALSE)
set(_lc_why "")
if(_lc_want_lccv STREQUAL "OFF")
  set(_lc_why "disabled (USE_LCCV=OFF)")
elseif(NOT CMAKE_SYSTEM_NAME STREQUAL Linux OR ANDROID)
  set(_lc_why "libcamera is Linux only (target is ${CMAKE_SYSTEM_NAME})")
else()
  find_libcamera()

  set(_lc_arm FALSE)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)")
    set(_lc_arm TRUE)
  endif()

  set(_lc_do_build FALSE)
  if(_lc_build STREQUAL "ON")
    set(_lc_do_build TRUE)
  elseif(_lc_build STREQUAL "AUTO")
    if(NOT LIBCAMERA_FOUND)
      set(_lc_do_build TRUE)
    elseif(_lc_arm AND NOT (LIBCAMERA_RPI_VC4 OR LIBCAMERA_RPI_PISP))
      message(STATUS "libcamera ${LIBCAMERA_VERSION} has no Raspberry Pi support, building our own")
      set(_lc_do_build TRUE)
    endif()
  endif()

  if(_lc_do_build)
    build_libcamera()
    if(LIBCAMERA_BUILT)
      set(LIBCAMERA_PREFIX "${LIBCAMERA_BUILT_PREFIX}" CACHE PATH "libcamera root dir" FORCE)
      set(LIBCAMERA_FOUND FALSE CACHE BOOL "libcamera found" FORCE)
      find_libcamera()
    else()
      if(_lc_build STREQUAL "ON")
        message(FATAL_ERROR "BUILD_LIBCAMERA=ON: ${LIBCAMERA_BUILD_ERROR}")
      endif()
      message(WARNING "${LIBCAMERA_BUILD_ERROR}")
      set(_lc_why "${LIBCAMERA_BUILD_ERROR}")
    endif()
  endif()

  if(NOT LIBCAMERA_FOUND AND NOT _lc_why)
    set(_lc_why "${LIBCAMERA_REJECTED}")
    if(NOT _lc_why)
      set(_lc_why "libcamera not found")
    endif()
    set(_lc_why "${_lc_why} (pkg-config libcamera; set LIBCAMERA_PREFIX or BUILD_LIBCAMERA=ON)")
  endif()
endif()

if(LIBCAMERA_FOUND)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)" AND NOT (LIBCAMERA_RPI_VC4 OR LIBCAMERA_RPI_PISP))
    message(WARNING "libcamera ${LIBCAMERA_VERSION} in ${LIBCAMERA_PREFIX} has no Raspberry Pi "
                    "IPA (rpi/vc4, rpi/pisp): the Pi camera will not work. Use BUILD_LIBCAMERA=ON.")
  endif()
  set(USE_LCCV ${_lc_want_lccv})
  if(USE_LCCV STREQUAL "AUTO")
    set(USE_LCCV ON)
  endif()
else()
  if(_lc_explicit)
    message(FATAL_ERROR "USE_LCCV=ON requested but libcamera is unusable: ${_lc_why}")
  endif()
  if(NOT _lc_why MATCHES "^disabled")
    message(WARNING "libcamera support (USE_LCCV) switched off: ${_lc_why}")
  else()
    message(STATUS "libcamera support ${_lc_why}")
  endif()
  set(USE_LCCV OFF)
endif()

if(USE_LCCV)
  include_directories(${LIBCAMERA_INCLUDE_DIRS})
  # LCCV/src/lccv.cpp includes <libcamera/libcamera/stream.h>, which only
  # resolves when <prefix>/include is on the include path (a system install).
  # For an install in another prefix add that root; #pragma once recognises it
  # as the same file as <libcamera/stream.h>. See BUGS lccv-hardcoded-include.
  if(EXISTS "${LIBCAMERA_PREFIX}/include/libcamera/libcamera/stream.h")
    include_directories(BEFORE "${LIBCAMERA_PREFIX}/include")
  endif()
  link_directories(${LIBCAMERA_LIBDIR})
  list(APPEND CMAKE_INSTALL_RPATH "${LIBCAMERA_LIBDIR}")

  # libevent, as LCCV uses it.
  pkg_check_modules(LIBCAMERA_EVENT QUIET libevent_pthreads)
  if(LIBCAMERA_EVENT_FOUND)
    set(LIBCAMERA_EVENT_LIBRARIES ${LIBCAMERA_EVENT_LINK_LIBRARIES})
  else()
    set(LIBCAMERA_EVENT_LIBRARIES event event_pthreads)
  endif()
  # Full paths: -lcamera would resolve to whatever copy the linker finds first.
  set(LIBCAMERA_ALL_LIBRARIES ${LIBCAMERA_LIBRARIES} ${LIBCAMERA_EVENT_LIBRARIES})
endif()
