# include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindLibcamera.cmake)
#
# Locates libcamera through pkg-config (optionally steered by an install
# prefix) and fills in LIBCAMERA_FOUND, LIBCAMERA_PREFIX, LIBCAMERA_VERSION,
# LIBCAMERA_INCLUDE_DIRS, LIBCAMERA_LIBRARIES (full paths, libcamera and
# libcamera-base), LIBCAMERA_LIBDIR and LIBCAMERA_PKGCONFIG_DIR.
#
# It also reports Raspberry Pi support of the found install:
#   LIBCAMERA_RPI_VC4   Pi 0-4: ipa_rpi_vc4 module + tuning files present
#   LIBCAMERA_RPI_PISP  Pi 5:   ipa_rpi_pisp module + tuning files present
# (the pipeline handlers are compiled into libcamera.so, so the IPA module and
# the tuning data are what can be checked on disk).
#
# Entry point on the command line, first configure or any later one:
#   -DLIBCAMERA_PREFIX=<install prefix>      (e.g. /opt/libcamera-0.5.2)
# The prefix may also be the directory containing libcamera.pc. Without it the
# default pkg-config search path is used. Changing LIBCAMERA_PREFIX re-resolves
# from scratch instead of reusing the cached result of the previous prefix.

# Oldest libcamera the bindings are built and tested against (LCCV and
# the camera binding use the public API only, which changes between 0.x releases).
set(LIBCAMERA_MIN_VERSION "0.5.2" CACHE STRING
    "oldest libcamera version accepted when searching for an installed one")
set(LIBCAMERA_TESTED_VERSION "0.5" CACHE STRING
    "libcamera major.minor the bindings were tested with (newer only warns)")

macro(find_libcamera)
  if(LIBCAMERA_PREFIX)
    get_filename_component(LIBCAMERA_PREFIX "${LIBCAMERA_PREFIX}" ABSOLUTE)
  endif(LIBCAMERA_PREFIX)

  # The cache entry LIBCAMERA_PREFIX is overwritten with the resolved prefix
  # below, so what the user asked for is compared against the snapshot taken
  # after the previous resolution, as find_opencv() does.
  if(NOT "${LIBCAMERA_PREFIX}" STREQUAL "${_LIBCAMERA_PREFIX_SEEN}")
    if(DEFINED _LIBCAMERA_PREFIX_SEEN)
      message(STATUS "libcamera location changed - re-resolving "
                     "(LIBCAMERA_PREFIX='${LIBCAMERA_PREFIX}')")
    endif(DEFINED _LIBCAMERA_PREFIX_SEEN)
    foreach(
      _lc_var
      FOUND
      VERSION
      INCLUDE_DIRS
      LIBRARIES
      LIBDIR
      PKGCONFIG_DIR
      RPI_VC4
      RPI_PISP)
      unset(LIBCAMERA_${_lc_var} CACHE)
      unset(LIBCAMERA_${_lc_var})
    endforeach(_lc_var)
    unset(__pkg_config_checked_LIBCAMERA_PC CACHE)
    get_cmake_property(_lc_cache CACHE_VARIABLES)
    foreach(_lc_var ${_lc_cache})
      if("${_lc_var}" MATCHES "^pkgcfg_lib_LIBCAMERA_PC_")
        unset(${_lc_var} CACHE)
      endif("${_lc_var}" MATCHES "^pkgcfg_lib_LIBCAMERA_PC_")
    endforeach(_lc_var)
    unset(_lc_cache)
    set(_lc_prefix_hint "${LIBCAMERA_PREFIX}")
    unset(LIBCAMERA_PREFIX CACHE)
    unset(LIBCAMERA_PREFIX)
  else("${LIBCAMERA_PREFIX}" STREQUAL "${_LIBCAMERA_PREFIX_SEEN}")
    # unchanged: still the request when nothing was resolved from it yet
    set(_lc_prefix_hint "${LIBCAMERA_PREFIX}")
  endif(NOT "${LIBCAMERA_PREFIX}" STREQUAL "${_LIBCAMERA_PREFIX_SEEN}")

  if(NOT LIBCAMERA_FOUND)
    find_package(PkgConfig QUIET)

    # Any layout of the requested prefix: install root or the .pc directory.
    set(_lc_pc_dirs)
    if(_lc_prefix_hint)
      if(NOT EXISTS "${_lc_prefix_hint}")
        message(WARNING "LIBCAMERA_PREFIX '${_lc_prefix_hint}' does not exist")
      endif(NOT EXISTS "${_lc_prefix_hint}")
      file(
        GLOB _lc_pc_files
        "${_lc_prefix_hint}/libcamera.pc"
        "${_lc_prefix_hint}/lib/pkgconfig/libcamera.pc"
        "${_lc_prefix_hint}/lib64/pkgconfig/libcamera.pc"
        "${_lc_prefix_hint}/lib/*/pkgconfig/libcamera.pc"
        "${_lc_prefix_hint}/share/pkgconfig/libcamera.pc")
      foreach(_lc_pc ${_lc_pc_files})
        get_filename_component(_lc_dir "${_lc_pc}" DIRECTORY)
        list(APPEND _lc_pc_dirs "${_lc_dir}")
      endforeach(_lc_pc)
      unset(_lc_pc_files)
    endif(_lc_prefix_hint)

    if(_lc_pc_dirs)
      # Only the requested prefix is searched: a hint must not silently fall
      # back to another install.
      set(_lc_env_old "$ENV{PKG_CONFIG_PATH}")
      set(_lc_libdir_old "$ENV{PKG_CONFIG_LIBDIR}")
      string(REPLACE ";" ":" _lc_path "${_lc_pc_dirs}")
      set(ENV{PKG_CONFIG_PATH} "${_lc_path}")
      set(ENV{PKG_CONFIG_LIBDIR} "${_lc_path}")
    endif(_lc_pc_dirs)

    if(PKG_CONFIG_FOUND AND (_lc_pc_dirs OR NOT _lc_prefix_hint))
      pkg_check_modules(LIBCAMERA_PC QUIET libcamera)
    endif(PKG_CONFIG_FOUND AND (_lc_pc_dirs OR NOT _lc_prefix_hint))

    if(LIBCAMERA_PC_FOUND AND LIBCAMERA_PC_VERSION VERSION_LESS LIBCAMERA_MIN_VERSION)
      message(STATUS "libcamera ${LIBCAMERA_PC_VERSION} rejected: older than "
                     "LIBCAMERA_MIN_VERSION ${LIBCAMERA_MIN_VERSION}")
      set(LIBCAMERA_PC_FOUND FALSE)
      set(LIBCAMERA_REJECTED "libcamera ${LIBCAMERA_PC_VERSION} is older than ${LIBCAMERA_MIN_VERSION}")
    endif()
    if(LIBCAMERA_PC_FOUND AND NOT LIBCAMERA_PC_VERSION MATCHES "^${LIBCAMERA_TESTED_VERSION}\\.")
      message(WARNING "libcamera ${LIBCAMERA_PC_VERSION}: bindings were tested with ${LIBCAMERA_TESTED_VERSION}.x only")
    endif()

    if(LIBCAMERA_PC_FOUND)
      pkg_get_variable(_lc_prefix libcamera prefix)
      pkg_get_variable(_lc_libdir libcamera libdir)
      list(GET LIBCAMERA_PC_LIBRARY_DIRS 0 _lc_first_libdir)
      if(NOT _lc_libdir)
        set(_lc_libdir "${_lc_first_libdir}")
      endif(NOT _lc_libdir)

      set(_lc_libs)
      foreach(_lc_name camera camera-base)
        find_library(_lc_lib_${_lc_name} NAMES ${_lc_name}
                     HINTS "${_lc_libdir}" NO_DEFAULT_PATH)
        if(NOT _lc_lib_${_lc_name})
          find_library(_lc_lib_${_lc_name} NAMES ${_lc_name})
        endif(NOT _lc_lib_${_lc_name})
        list(APPEND _lc_libs "${_lc_lib_${_lc_name}}")
        unset(_lc_lib_${_lc_name} CACHE)
      endforeach(_lc_name)

      # A libcamera built with its libpisp subproject installs libpisp.so
      # beside itself, but meson install strips every rpath, so libcamera.so
      # alone cannot find it at run time. Linking it too puts it in the
      # consumer's NEEDED list, where the consumer's own RUNPATH applies.
      find_library(_lc_lib_pisp NAMES pisp HINTS "${_lc_libdir}" NO_DEFAULT_PATH)
      # --no-as-needed: no symbol of it is used directly, the linker would drop it.
      if(_lc_lib_pisp)
        list(APPEND _lc_libs -Wl,--no-as-needed "${_lc_lib_pisp}" -Wl,--as-needed)
      endif(_lc_lib_pisp)
      unset(_lc_lib_pisp CACHE)

      set(LIBCAMERA_VERSION "${LIBCAMERA_PC_VERSION}"
          CACHE STRING "libcamera version" FORCE)
      set(LIBCAMERA_INCLUDE_DIRS "${LIBCAMERA_PC_INCLUDE_DIRS}"
          CACHE STRING "libcamera include directories" FORCE)
      set(LIBCAMERA_LIBRARIES "${_lc_libs}"
          CACHE STRING "libcamera libraries (full paths)" FORCE)
      set(LIBCAMERA_LIBDIR "${_lc_libdir}"
          CACHE PATH "libcamera library directory" FORCE)
      set(LIBCAMERA_PREFIX "${_lc_prefix}" CACHE PATH "libcamera root dir" FORCE)
      if(_lc_pc_dirs)
        list(GET _lc_pc_dirs 0 _lc_pcdir)
      else(_lc_pc_dirs)
        set(_lc_pcdir "")
      endif(_lc_pc_dirs)
      set(LIBCAMERA_PKGCONFIG_DIR "${_lc_pcdir}"
          CACHE PATH "directory of libcamera.pc when outside the default path"
          FORCE)

      # --- Raspberry Pi support -------------------------------------------
      foreach(_lc_pi vc4 pisp)
        string(TOUPPER "${_lc_pi}" _lc_PI)
        file(
          GLOB _lc_ipa
          "${_lc_libdir}/libcamera/ipa_rpi_${_lc_pi}*.so"
          "${_lc_libdir}/libcamera/ipa/ipa_rpi_${_lc_pi}*.so"
          "${_lc_prefix}/lib*/libcamera/ipa_rpi_${_lc_pi}*.so"
          "${_lc_prefix}/lib*/libcamera/ipa/ipa_rpi_${_lc_pi}*.so"
          "${_lc_prefix}/lib/*/libcamera/ipa_rpi_${_lc_pi}*.so"
          "${_lc_prefix}/lib/*/libcamera/ipa/ipa_rpi_${_lc_pi}*.so")
        file(GLOB _lc_tuning
             "${_lc_prefix}/share/libcamera/ipa/rpi/${_lc_pi}/*.json")
        if(_lc_ipa AND _lc_tuning)
          set(LIBCAMERA_RPI_${_lc_PI} TRUE
              CACHE BOOL "libcamera has the Raspberry Pi ${_lc_pi} IPA" FORCE)
        else(_lc_ipa AND _lc_tuning)
          set(LIBCAMERA_RPI_${_lc_PI} FALSE
              CACHE BOOL "libcamera has the Raspberry Pi ${_lc_pi} IPA" FORCE)
        endif(_lc_ipa AND _lc_tuning)
        unset(_lc_ipa)
        unset(_lc_tuning)
      endforeach(_lc_pi)

      set(LIBCAMERA_FOUND TRUE CACHE BOOL "libcamera found" FORCE)
      message(STATUS "libcamera ${LIBCAMERA_VERSION} found: ${LIBCAMERA_PREFIX} "
                     "(libdir ${LIBCAMERA_LIBDIR}, rpi/vc4: ${LIBCAMERA_RPI_VC4}, "
                     "rpi/pisp: ${LIBCAMERA_RPI_PISP})")
    else(LIBCAMERA_PC_FOUND)
      message(STATUS "libcamera not found")
    endif(LIBCAMERA_PC_FOUND)

    if(_lc_pc_dirs)
      set(ENV{PKG_CONFIG_PATH} "${_lc_env_old}")
      set(ENV{PKG_CONFIG_LIBDIR} "${_lc_libdir_old}")
    endif(_lc_pc_dirs)
    unset(_lc_pc_dirs)
  endif(NOT LIBCAMERA_FOUND)

  # LIBCAMERA_PREFIX is only ever the user's request or the resolved prefix.
  set(_LIBCAMERA_PREFIX_SEEN "${LIBCAMERA_PREFIX}" CACHE INTERNAL "")
  unset(_lc_prefix_hint)

  # Consumers (LCCV runs its own pkg_check_modules(libcamera)) must see the
  # same install, in every configure and not only in the one that resolved it.
  if(LIBCAMERA_FOUND AND LIBCAMERA_PKGCONFIG_DIR)
    set(ENV{PKG_CONFIG_PATH} "${LIBCAMERA_PKGCONFIG_DIR}:$ENV{PKG_CONFIG_PATH}")
  endif(LIBCAMERA_FOUND AND LIBCAMERA_PKGCONFIG_DIR)
endmacro(find_libcamera)
