# include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildLibcamera.cmake)
#
# build_libcamera()
#
# Builds and installs libcamera with meson, at CONFIGURE time, into
# ${LIBCAMERA_INSTALL_PREFIX}. It is not an ExternalProject on purpose: LCCV
# runs pkg_check_modules(libcamera REQUIRED) while it is being configured and
# the headers and libcamera.pc must exist by then. A stamp file (in the build dir) makes later
# configures a no-op until one of the build inputs changes.
#
# Sets LIBCAMERA_BUILT TRUE and LIBCAMERA_PREFIX (to the install prefix) on
# success; on failure LIBCAMERA_BUILT is FALSE and LIBCAMERA_BUILD_ERROR holds a
# message listing what is missing or what failed (never a FATAL_ERROR: the caller
# decides, because an AUTO build only warns).
#
# Variables (all optional):
#   LIBCAMERA_SOURCES_ROOT     local checkout; default ~/Sources/libcamera if it
#                              exists, else a clone of LIBCAMERA_GIT_URL
#   LIBCAMERA_GIT_URL          default https://git.libcamera.org/libcamera/libcamera.git
#   LIBCAMERA_GIT_TAG          default v0.5.2 (upstream has rpi/vc4 and rpi/pisp
#                              since 0.3; LCCV uses no Raspberry Pi fork API)
#   LIBCAMERA_INSTALL_PREFIX   default ${CMAKE_BINARY_DIR}/deps/libcamera
#   LIBCAMERA_PIPELINES        default rpi/vc4,rpi/pisp,uvcvideo,simple
#   LIBCAMERA_IPAS             default rpi/vc4,rpi/pisp,simple
#   LIBCAMERA_BUILD_JOBS       default 4
#   LIBCAMERA_MESON_ARGS       extra meson setup arguments
#
# Cross builds (CMAKE_CROSSCOMPILING): a meson cross file is generated from the
# CMake toolchain (compilers, CMAKE_SYSROOT, CMAKE_SYSTEM_PROCESSOR) next to
# the build directory; dependencies are looked up inside CMAKE_SYSROOT.

set(LIBCAMERA_GIT_URL "https://git.libcamera.org/libcamera/libcamera.git"
    CACHE STRING "libcamera git repository used when no source tree is given")
set(LIBCAMERA_GIT_TAG "v0.5.2" CACHE STRING "libcamera tag to clone")
set(LIBCAMERA_SOURCES_ROOT "" CACHE PATH
    "libcamera source tree (default: ~/Sources/libcamera if present, else a clone)")
set(LIBCAMERA_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/deps/libcamera"
    CACHE PATH "where BUILD_LIBCAMERA installs libcamera")
set(LIBCAMERA_PIPELINES "rpi/vc4,rpi/pisp,uvcvideo,simple"
    CACHE STRING "meson -Dpipelines= for BUILD_LIBCAMERA")
set(LIBCAMERA_IPAS "rpi/vc4,rpi/pisp,simple"
    CACHE STRING "meson -Dipas= for BUILD_LIBCAMERA")
set(LIBCAMERA_BUILD_JOBS "4" CACHE STRING "parallel jobs for the libcamera build")
set(LIBCAMERA_MESON_ARGS "" CACHE STRING "extra meson setup arguments")

# Appends each tool or module that is missing to _lc_missing.
macro(_libcamera_check_tools)
  set(_lc_missing)

  find_program(LIBCAMERA_MESON meson)
  # A real binary from the system directories first: ninja on the PATH may be
  # depot_tools' launcher script.
  find_program(LIBCAMERA_NINJA NAMES ninja ninja-build samu
               PATHS /usr/bin /usr/local/bin /bin NO_DEFAULT_PATH)
  find_program(LIBCAMERA_NINJA NAMES ninja ninja-build samu)
  find_program(LIBCAMERA_PYTHON NAMES python3 python)
  find_program(LIBCAMERA_PKG_CONFIG NAMES pkg-config pkgconf)
  if(NOT LIBCAMERA_MESON)
    list(APPEND _lc_missing "meson >= 0.63 (pip install meson | apt install meson)")
  endif(NOT LIBCAMERA_MESON)
  if(NOT LIBCAMERA_NINJA)
    list(APPEND _lc_missing "ninja (apt install ninja-build)")
  endif(NOT LIBCAMERA_NINJA)
  if(NOT LIBCAMERA_PKG_CONFIG)
    list(APPEND _lc_missing "pkg-config")
  endif(NOT LIBCAMERA_PKG_CONFIG)
  if(NOT LIBCAMERA_PYTHON)
    list(APPEND _lc_missing "python3")
  else(NOT LIBCAMERA_PYTHON)
    foreach(_lc_mod yaml ply jinja2)
      execute_process(
        COMMAND "${LIBCAMERA_PYTHON}" -c "import ${_lc_mod}"
        RESULT_VARIABLE _lc_rv OUTPUT_QUIET ERROR_QUIET)
      if(_lc_rv)
        list(APPEND _lc_missing
             "python module '${_lc_mod}' (apt install python3-${_lc_mod} | pip install ${_lc_mod})")
      endif(_lc_rv)
    endforeach(_lc_mod)
  endif(NOT LIBCAMERA_PYTHON)

  if(LIBCAMERA_MESON)
    execute_process(COMMAND "${LIBCAMERA_MESON}" --version
                    OUTPUT_VARIABLE _lc_mver OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)
    if(_lc_mver AND _lc_mver VERSION_LESS 0.63)
      list(APPEND _lc_missing "meson >= 0.63 (found ${_lc_mver})")
    endif(_lc_mver AND _lc_mver VERSION_LESS 0.63)
  endif(LIBCAMERA_MESON)

  if(LIBCAMERA_PKG_CONFIG AND NOT CMAKE_CROSSCOMPILING)
    # libevent is needed by LCCV and libcamera's apps. libyaml and libpisp
    # have meson wraps (downloaded, needs network) so they are not required here.
    pkg_check_modules(_LC_EVENT QUIET libevent_pthreads)
    if(NOT _LC_EVENT_FOUND)
      list(APPEND _lc_missing "libevent with pthreads (apt install libevent-dev)")
    endif(NOT _LC_EVENT_FOUND)
    pkg_check_modules(_LC_CRYPTO QUIET gnutls)
    if(NOT _LC_CRYPTO_FOUND)
      pkg_check_modules(_LC_CRYPTO QUIET libcrypto)
    endif(NOT _LC_CRYPTO_FOUND)
    find_program(LIBCAMERA_OPENSSL openssl)
    if(NOT _LC_CRYPTO_FOUND OR NOT LIBCAMERA_OPENSSL)
      list(APPEND _lc_missing
           "openssl binary and libssl-dev or gnutls-dev (IPA module signing)")
    endif(NOT _LC_CRYPTO_FOUND OR NOT LIBCAMERA_OPENSSL)
    pkg_check_modules(_LC_YAML QUIET yaml-0.1)
    pkg_check_modules(_LC_PISP QUIET libpisp)
    if(NOT _LC_YAML_FOUND OR NOT _LC_PISP_FOUND)
      find_program(LIBCAMERA_GIT git)
      if(NOT LIBCAMERA_GIT)
        list(APPEND _lc_missing
             "git (meson downloads libyaml/libpisp subprojects when the packages are absent)")
      else(NOT LIBCAMERA_GIT)
        message(STATUS "libcamera: libyaml/libpisp not installed, meson will fetch them (network)")
      endif(NOT LIBCAMERA_GIT)
    endif(NOT _LC_YAML_FOUND OR NOT _LC_PISP_FOUND)
  endif(LIBCAMERA_PKG_CONFIG AND NOT CMAKE_CROSSCOMPILING)
endmacro(_libcamera_check_tools)

# Writes ${LIBCAMERA_CROSS_FILE} for CMAKE_CROSSCOMPILING builds.
macro(_libcamera_cross_file)
  set(_lc_cpu "${CMAKE_SYSTEM_PROCESSOR}")
  if(_lc_cpu MATCHES "^(arm64|aarch64)")
    set(_lc_family aarch64)
    set(_lc_cpu aarch64)
  elseif(_lc_cpu MATCHES "^arm")
    set(_lc_family arm)
  else()
    set(_lc_family "${_lc_cpu}")
  endif()
  set(_lc_sysroot_args "")
  set(_lc_pc_dirs "")
  if(CMAKE_SYSROOT)
    set(_lc_sysroot_args "'--sysroot=${CMAKE_SYSROOT}'")
    set(_lc_pc_dirs "'${CMAKE_SYSROOT}/usr/lib/pkgconfig', '${CMAKE_SYSROOT}/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/pkgconfig', '${CMAKE_SYSROOT}/usr/share/pkgconfig'")
  endif(CMAKE_SYSROOT)
  set(LIBCAMERA_CROSS_FILE "${CMAKE_BINARY_DIR}/libcamera-cross.ini")
  file(
    WRITE "${LIBCAMERA_CROSS_FILE}"
    "[binaries]
c = '${CMAKE_C_COMPILER}'
cpp = '${CMAKE_CXX_COMPILER}'
pkg-config = '${LIBCAMERA_PKG_CONFIG}'

[built-in options]
c_args = [${_lc_sysroot_args}]
cpp_args = [${_lc_sysroot_args}]
c_link_args = [${_lc_sysroot_args}]
cpp_link_args = [${_lc_sysroot_args}]
pkg_config_path = [${_lc_pc_dirs}]

[host_machine]
system = 'linux'
cpu_family = '${_lc_family}'
cpu = '${_lc_cpu}'
endian = 'little'
")
  message(STATUS "libcamera: cross file ${LIBCAMERA_CROSS_FILE} (cpu_family ${_lc_family})")
endmacro(_libcamera_cross_file)

function(build_libcamera)
  set(LIBCAMERA_BUILT FALSE PARENT_SCOPE)
  set(LIBCAMERA_BUILD_ERROR "" PARENT_SCOPE)
  find_package(PkgConfig QUIET)

  _libcamera_check_tools()
  if(_lc_missing)
    string(REPLACE ";" "\n  - " _lc_list "${_lc_missing}")
    set(LIBCAMERA_BUILD_ERROR
        "cannot build libcamera, missing:\n  - ${_lc_list}" PARENT_SCOPE)
    return()
  endif(_lc_missing)

  # --- source tree ------------------------------------------------------
  set(_lc_src "${LIBCAMERA_SOURCES_ROOT}")
  if(NOT _lc_src AND EXISTS "$ENV{HOME}/Sources/libcamera/meson.build")
    set(_lc_src "$ENV{HOME}/Sources/libcamera")
  endif(NOT _lc_src AND EXISTS "$ENV{HOME}/Sources/libcamera/meson.build")
  if(_lc_src AND NOT EXISTS "${_lc_src}/meson.build")
    set(LIBCAMERA_BUILD_ERROR
        "LIBCAMERA_SOURCES_ROOT '${_lc_src}' has no meson.build" PARENT_SCOPE)
    return()
  endif(_lc_src AND NOT EXISTS "${_lc_src}/meson.build")
  if(NOT _lc_src)
    find_program(LIBCAMERA_GIT git)
    if(NOT LIBCAMERA_GIT)
      set(LIBCAMERA_BUILD_ERROR
          "cannot build libcamera: no LIBCAMERA_SOURCES_ROOT and git is missing"
          PARENT_SCOPE)
      return()
    endif(NOT LIBCAMERA_GIT)
    set(_lc_src "${CMAKE_BINARY_DIR}/deps/libcamera-src")
    if(NOT EXISTS "${_lc_src}/meson.build")
      message(STATUS "libcamera: cloning ${LIBCAMERA_GIT_URL} ${LIBCAMERA_GIT_TAG}")
      execute_process(
        COMMAND "${LIBCAMERA_GIT}" clone --depth 1 --branch "${LIBCAMERA_GIT_TAG}"
                "${LIBCAMERA_GIT_URL}" "${_lc_src}"
        RESULT_VARIABLE _lc_rv ERROR_VARIABLE _lc_err)
      if(_lc_rv)
        file(REMOVE_RECURSE "${_lc_src}")
        set(LIBCAMERA_BUILD_ERROR "git clone of libcamera failed: ${_lc_err}"
            PARENT_SCOPE)
        return()
      endif(_lc_rv)
    endif(NOT EXISTS "${_lc_src}/meson.build")
  endif(NOT _lc_src)

  # --- options and stamp -------------------------------------------------
  set(_lc_build "${CMAKE_BINARY_DIR}/deps/libcamera-build")
  set(_lc_prefix "${LIBCAMERA_INSTALL_PREFIX}")
  set(_lc_args
      "--prefix=${_lc_prefix}" --libdir=lib --buildtype=release
      --default-library=shared "-Dpipelines=${LIBCAMERA_PIPELINES}"
      "-Dipas=${LIBCAMERA_IPAS}" -Dcam=disabled -Dqcam=disabled
      -Dgstreamer=disabled -Dpycamera=disabled -Dlc-compliance=disabled
      -Ddocumentation=disabled -Dtracing=disabled -Dv4l2=disabled
      -Dandroid=disabled -Dtest=false
      # libcamera 0.5.2 does not build with GCC 16's -Werror=array-bounds
      -Dwerror=false)
  if(CMAKE_CROSSCOMPILING)
    _libcamera_cross_file()
    list(APPEND _lc_args "--cross-file=${LIBCAMERA_CROSS_FILE}")
  endif(CMAKE_CROSSCOMPILING)
  if(LIBCAMERA_MESON_ARGS)
    separate_arguments(_lc_extra UNIX_COMMAND "${LIBCAMERA_MESON_ARGS}")
    list(APPEND _lc_args ${_lc_extra})
  endif(LIBCAMERA_MESON_ARGS)

  string(SHA1 _lc_hash "${_lc_src};${_lc_args}")
  set(_lc_stamp "${_lc_build}/.qjs-opencv-stamp-${_lc_hash}")

  file(GLOB _lc_pcs "${_lc_prefix}/lib/pkgconfig/libcamera.pc")
  if(NOT EXISTS "${_lc_stamp}" OR NOT _lc_pcs)
    message(STATUS "libcamera: building ${_lc_src} -> ${_lc_prefix} (this takes several minutes)")
    set(_lc_cmd_setup "${LIBCAMERA_MESON}" setup)
    if(EXISTS "${_lc_build}/build.ninja")
      list(APPEND _lc_cmd_setup --reconfigure)
    endif(EXISTS "${_lc_build}/build.ninja")
    # meson takes ninja from $NINJA. Never a wrapper script (depot_tools' ninja
    # is a python launcher that puts its own python3 first in PATH, which breaks
    # the code generators), see the find_program above.
    set(ENV{NINJA} "${LIBCAMERA_NINJA}")

    execute_process(
      COMMAND ${_lc_cmd_setup} ${_lc_args} "${_lc_build}" "${_lc_src}"
      RESULT_VARIABLE _lc_rv OUTPUT_VARIABLE _lc_out ERROR_VARIABLE _lc_err
      TIMEOUT 1800)
    if(_lc_rv)
      set(LIBCAMERA_BUILD_ERROR "meson setup of libcamera failed:\n${_lc_out}${_lc_err}"
          PARENT_SCOPE)
      return()
    endif(_lc_rv)
    foreach(_lc_step compile install)
      set(_lc_jobs)
      if(_lc_step STREQUAL compile)
        set(_lc_jobs -j ${LIBCAMERA_BUILD_JOBS})
      endif(_lc_step STREQUAL compile)
      execute_process(
        COMMAND "${LIBCAMERA_MESON}" ${_lc_step} -C "${_lc_build}" ${_lc_jobs}
        RESULT_VARIABLE _lc_rv OUTPUT_VARIABLE _lc_out ERROR_VARIABLE _lc_err
        TIMEOUT 7200)
      if(_lc_rv)
        string(LENGTH "${_lc_out}${_lc_err}" _lc_len)
        set(_lc_all "${_lc_out}${_lc_err}")
        if(_lc_len GREATER 4000)
          math(EXPR _lc_from "${_lc_len} - 4000")
          string(SUBSTRING "${_lc_all}" ${_lc_from} 4000 _lc_all)
        endif(_lc_len GREATER 4000)
        set(LIBCAMERA_BUILD_ERROR "meson ${_lc_step} of libcamera failed:\n${_lc_all}"
            PARENT_SCOPE)
        return()
      endif(_lc_rv)
    endforeach(_lc_step)
    file(WRITE "${_lc_stamp}" "${_lc_args}\n")
  else(NOT EXISTS "${_lc_stamp}" OR NOT _lc_pcs)
    message(STATUS "libcamera: already built in ${_lc_prefix}")
  endif(NOT EXISTS "${_lc_stamp}" OR NOT _lc_pcs)

  set(LIBCAMERA_BUILT TRUE PARENT_SCOPE)
  set(LIBCAMERA_BUILT_PREFIX "${_lc_prefix}" PARENT_SCOPE)
endfunction(build_libcamera)
