# include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/MoreFunctions.cmake)
#
# Locates OpenCV via its exported OpenCVConfig.cmake (falling back to
# pkg-config), and populates OPENCV_FOUND, OPENCV_PREFIX, OPENCV_VERSION,
# OPENCV_INCLUDE_DIRS, OPENCV_LIBRARIES, OPENCV_LIBDIR.
#
# Two equivalent entry points are supported, both on the first configure of a
# build dir and on any later reconfigure:
#   -DOPENCV_PREFIX=<install prefix>              (e.g. /opt/opencv-5.0.0-x86_64)
#   -DOpenCV_DIR=<prefix>/lib/cmake/opencv4        (CMake's own find_package variable)
# Either may point at an install root or directly at the directory containing
# OpenCVConfig.cmake - both forms are tried. Re-running cmake with a changed
# OPENCV_PREFIX or OpenCV_DIR re-resolves OpenCV from scratch instead of
# reusing stale cached results; re-running with nothing changed is a fast
# no-op.

macro(find_opencv)
  # --- normalize inputs -------------------------------------------------
  if(OPENCV_PREFIX)
    get_filename_component(OPENCV_PREFIX "${OPENCV_PREFIX}" ABSOLUTE)
  endif(OPENCV_PREFIX)
  if(OpenCV_DIR)
    get_filename_component(OpenCV_DIR "${OpenCV_DIR}" ABSOLUTE)
  endif(OpenCV_DIR)

  # --- change detection ---------------------------------------------------
  # OPENCV_PREFIX and OpenCV_DIR are tracked and compared *independently*,
  # against snapshots taken *after* the previous resolution completed (see
  # the bottom of this macro), not against their raw values on entry:
  # find_package(OpenCV CONFIG) writes its own resolved directory back into
  # OpenCV_DIR as a side effect of succeeding, and this macro writes a
  # normalized OPENCV_PREFIX from that result. Comparing against a
  # pre-resolution snapshot would mistake that write-back for a user-driven
  # change on every subsequent run. Tracking the two separately (rather than
  # as one combined value) matters too: if the user only ever repointed
  # OpenCV_DIR, OPENCV_PREFIX still holds its own last resolved value, and
  # blindly searching both would let that stale, unrelated OPENCV_PREFIX
  # silently outrank the OpenCV_DIR the user actually just changed.
  set(_opencv_prefix_changed FALSE)
  set(_opencv_dir_changed FALSE)
  if(NOT "${OPENCV_PREFIX}" STREQUAL "${_OPENCV_PREFIX_SEEN}")
    set(_opencv_prefix_changed TRUE)
  endif(NOT "${OPENCV_PREFIX}" STREQUAL "${_OPENCV_PREFIX_SEEN}")
  if(NOT "${OpenCV_DIR}" STREQUAL "${_OPENCV_DIR_SEEN}")
    set(_opencv_dir_changed TRUE)
  endif(NOT "${OpenCV_DIR}" STREQUAL "${_OPENCV_DIR_SEEN}")

  # Preserve the requested values for our own PATHS search below, since one
  # or both are about to get cleared.
  set(_opencv_prefix_hint "${OPENCV_PREFIX}")
  set(_opencv_dir_hint "${OpenCV_DIR}")

  if(_opencv_prefix_changed OR _opencv_dir_changed)
    if(DEFINED _OPENCV_PREFIX_SEEN OR DEFINED _OPENCV_DIR_SEEN)
      message(STATUS "OpenCV location changed - re-resolving "
                     "(OpenCV_DIR='${OpenCV_DIR}' OPENCV_PREFIX='${OPENCV_PREFIX}')")
    endif(DEFINED _OPENCV_PREFIX_SEEN OR DEFINED _OPENCV_DIR_SEEN)

    # Whichever of the two did *not* just change is presumably a stale
    # leftover from the previous resolution's own write-back, not something
    # the user is currently asking for - drop it from the search below so it
    # can't outrank the one that did change.
    if(_opencv_dir_changed AND NOT _opencv_prefix_changed)
      set(_opencv_prefix_hint "")
    endif(_opencv_dir_changed AND NOT _opencv_prefix_changed)
    if(_opencv_prefix_changed AND NOT _opencv_dir_changed)
      set(_opencv_dir_hint "")
    endif(_opencv_prefix_changed AND NOT _opencv_dir_changed)

    unset(OPENCV_FOUND CACHE)

    unset_all(
      OPENCV_CFLAGS
      OPENCV_INCLUDEDIR
      OPENCV_INCLUDE_DIRS
      OPENCV_LDFLAGS
      OPENCV_LIBDIR
      OPENCV_LIBRARIES
      OPENCV_LIBDIRS
      OPENCV_MODULE_NAME
      OPENCV_STATIC_CFLAGS
      OPENCV_STATIC_INCLUDE_DIRS
      OPENCV_STATIC_LDFLAGS
      OPENCV_STATIC_LIBRARIES
      OPENCV_STATIC_LIBDIRS
      OPENCV_VERSION
      OPENCV_XFEATURES2D_HPP
      pkgcfg_lib_OPENCV_opencv_aruco
      pkgcfg_lib_OPENCV_opencv_bgsegm
      pkgcfg_lib_OPENCV_opencv_bioinspired
      pkgcfg_lib_OPENCV_opencv_calib3d
      pkgcfg_lib_OPENCV_opencv_ccalib
      pkgcfg_lib_OPENCV_opencv_core
      pkgcfg_lib_OPENCV_opencv_datasets
      pkgcfg_lib_OPENCV_opencv_dnn
      pkgcfg_lib_OPENCV_opencv_dnn_objdetect
      pkgcfg_lib_OPENCV_opencv_dnn_superres
      pkgcfg_lib_OPENCV_opencv_dpm
      pkgcfg_lib_OPENCV_opencv_face
      pkgcfg_lib_OPENCV_opencv_features2d
      pkgcfg_lib_OPENCV_opencv_flann
      pkgcfg_lib_OPENCV_opencv_freetype
      pkgcfg_lib_OPENCV_opencv_fuzzy
      pkgcfg_lib_OPENCV_opencv_hdf
      pkgcfg_lib_OPENCV_opencv_hfs
      pkgcfg_lib_OPENCV_opencv_highgui
      pkgcfg_lib_OPENCV_opencv_imgcodecs
      pkgcfg_lib_OPENCV_opencv_img_hash
      pkgcfg_lib_OPENCV_opencv_imgproc
      pkgcfg_lib_OPENCV_opencv_line_descriptor
      pkgcfg_lib_OPENCV_opencv_ml
      pkgcfg_lib_OPENCV_opencv_objdetect
      pkgcfg_lib_OPENCV_opencv_optflow
      pkgcfg_lib_OPENCV_opencv_phase_unwrapping
      pkgcfg_lib_OPENCV_opencv_photo
      pkgcfg_lib_OPENCV_opencv_plot
      pkgcfg_lib_OPENCV_opencv_quality
      pkgcfg_lib_OPENCV_opencv_reg
      pkgcfg_lib_OPENCV_opencv_rgbd
      pkgcfg_lib_OPENCV_opencv_saliency
      pkgcfg_lib_OPENCV_opencv_shape
      pkgcfg_lib_OPENCV_opencv_stereo
      pkgcfg_lib_OPENCV_opencv_stitching
      pkgcfg_lib_OPENCV_opencv_structured_light
      pkgcfg_lib_OPENCV_opencv_superres
      pkgcfg_lib_OPENCV_opencv_surface_matching
      pkgcfg_lib_OPENCV_opencv_text
      pkgcfg_lib_OPENCV_opencv_tracking
      pkgcfg_lib_OPENCV_opencv_video
      pkgcfg_lib_OPENCV_opencv_videoio
      pkgcfg_lib_OPENCV_opencv_videostab
      pkgcfg_lib_OPENCV_opencv_viz
      pkgcfg_lib_OPENCV_opencv_ximgproc
      pkgcfg_lib_OPENCV_opencv_xobjdetect
      pkgcfg_lib_OPENCV_opencv_xphoto
      pkgcfg_lib_OPENCV_opencv_alphamat
      pkgcfg_lib_OPENCV_opencv_gapi
      pkgcfg_lib_OPENCV_opencv_intensity_transform
      pkgcfg_lib_OPENCV_opencv_mcc
      pkgcfg_lib_OPENCV_opencv_rapid
      pkgcfg_lib_OPENCV_opencv_sfm
      pkgcfg_lib_OPENCV_opencv_xfeatures2d)

    # Fully clear OPENCV_PREFIX/OpenCV_DIR - both the cache entries *and* the
    # normal-scope variables the ABSOLUTE-normalization above created, which
    # would otherwise still shadow the cache for the rest of this run.
    # find_package(CONFIG) treats <Package>_DIR as a priority hint independent
    # of (and searched before) the PATHS argument below, so leaving a stale
    # value in place - cache or not - would keep silently winning over a
    # freshly-requested location. _opencv_prefix_hint/_opencv_dir_hint
    # (captured above) carry the values forward for our own PATHS search.
    unset(OPENCV_PREFIX CACHE)
    unset(OPENCV_PREFIX)
    unset(OpenCV_DIR CACHE)
    unset(OpenCV_DIR)
  endif(_opencv_prefix_changed OR _opencv_dir_changed)
  unset(_opencv_prefix_changed)
  unset(_opencv_dir_changed)

  if(NOT OPENCV_FOUND)
    message(STATUS "Finding OpenCV library")

    if(_opencv_prefix_hint AND NOT EXISTS "${_opencv_prefix_hint}")
      message(WARNING "OPENCV_PREFIX '${_opencv_prefix_hint}' does not exist")
    endif(_opencv_prefix_hint AND NOT EXISTS "${_opencv_prefix_hint}")
    if(_opencv_dir_hint AND NOT EXISTS "${_opencv_dir_hint}")
      message(WARNING "OpenCV_DIR '${_opencv_dir_hint}' does not exist")
    endif(_opencv_dir_hint AND NOT EXISTS "${_opencv_dir_hint}")

    # Accept OPENCV_PREFIX/OpenCV_DIR as either an install root or the exact
    # directory containing OpenCVConfig.cmake - try every plausible layout so
    # a wide range of user input works without needing to know which form is
    # expected. Uses the hint variables (not OPENCV_PREFIX/OpenCV_DIR
    # directly, which have just been cleared above) so only a freshly
    # requested location is searched, never whatever a stale side effect
    # left behind.
    set(_opencv_search_paths)
    foreach(_opencv_root IN LISTS _opencv_prefix_hint _opencv_dir_hint)
      if(_opencv_root)
        list(
          APPEND
          _opencv_search_paths
          "${_opencv_root}/lib/cmake/opencv4"
          "${_opencv_root}/lib64/cmake/opencv4"
          "${_opencv_root}/lib/cmake"
          "${_opencv_root}/share/OpenCV"
          "${_opencv_root}")
      endif(_opencv_root)
    endforeach(_opencv_root)
    if(_opencv_search_paths)
      list(REMOVE_DUPLICATES _opencv_search_paths)
    endif(_opencv_search_paths)

    if(_opencv_search_paths)
      if(_opencv_prefix_hint)
        list(APPEND CMAKE_PREFIX_PATH "${_opencv_prefix_hint}")
      endif(_opencv_prefix_hint)
      if(_opencv_dir_hint)
        list(APPEND CMAKE_PREFIX_PATH "${_opencv_dir_hint}")
      endif(_opencv_dir_hint)
      find_package(OpenCV CONFIG QUIET PATHS ${_opencv_search_paths}
                   NO_DEFAULT_PATH)
    else(_opencv_search_paths)
      find_package(OpenCV CONFIG QUIET)
    endif(_opencv_search_paths)
    unset(_opencv_search_paths)

    if(OpenCV_VERSION)
      set(OPENCV_VERSION "${OpenCV_VERSION}" CACHE PATH "OpenCV version" FORCE)
      set(OPENCV_INCLUDE_DIRS "${OpenCV_INCLUDE_DIRS}"
          CACHE PATH "OpenCV include directories" FORCE)
      set(OPENCV_LIBRARIES "${OpenCV_LIBS}" CACHE PATH "OpenCV libraries" FORCE)
      #set(OPENCV_LINK_FLAGS "-Wl,-rpath,${OPENCV_LIBDIR} -L${OPENCV_LIBDIR}" CACHE STRING "OpenCV link flags")

      # OpenCV_INSTALL_PATH is exported by OpenCVConfig.cmake and computed
      # from the config file's own real location - authoritative regardless
      # of whether the user passed an install root or the exact config dir,
      # unlike deriving it by pattern-matching OPENCV_PREFIX/OPENCV_LIBDIR.
      if(OpenCV_INSTALL_PATH)
        set(OPENCV_PREFIX "${OpenCV_INSTALL_PATH}" CACHE PATH "OpenCV root dir" FORCE)
        set(OPENCV_LIBDIR "${OpenCV_INSTALL_PATH}/lib"
            CACHE PATH "OpenCV library directory" FORCE)
      endif(OpenCV_INSTALL_PATH)

      set(OPENCV_FOUND TRUE CACHE BOOL "OpenCV found" FORCE)
    endif(OpenCV_VERSION)

    if(NOT OPENCV_FOUND)
      set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
      pkg_search_module(OPENCV REQUIRED opencv opencv4)

      if(OPENCV_FOUND)
        message(STATUS "OpenCV found via pkg-config")
      endif(OPENCV_FOUND)
    endif(NOT OPENCV_FOUND)

    if(NOT OPENCV_PREFIX)
      set(OPENCV_PREFIX "${_opencv_prefix_hint}"
          CACHE PATH "OpenCV install directory" FORCE)
    endif(NOT OPENCV_PREFIX)
    if(NOT OPENCV_LIBDIR)
      set(OPENCV_LIBDIR "${OPENCV_PREFIX}/lib"
          CACHE PATH "OpenCV library directory" FORCE)
    endif(NOT OPENCV_LIBDIR)
    
    if(NOT OPENCV_INCLUDE_DIRS)
      if(OPENCV_LIBDIR)
        string(REGEX REPLACE "/lib.*" "/include" OPENCV_INCLUDE_DIRS "${OPENCV_LIBDIR}")
        set(OPENCV_INCLUDE_DIRS "${OPENCV_INCLUDE_DIRS}" CACHE PATH "OpenCV include directory" FORCE)
      endif(OPENCV_LIBDIR)
    endif(NOT OPENCV_INCLUDE_DIRS)

    if("${OPENCV_INCLUDE_DIRS}" MATCHES "/include$")
      if(EXISTS "${OPENCV_INCLUDE_DIRS}/opencv5")
        set(OPENCV_INCLUDE_DIRS "${OPENCV_INCLUDE_DIRS}/opencv5"  CACHE PATH "OpenCV include directory" FORCE)
      else(EXISTS "${OPENCV_INCLUDE_DIRS}/opencv5")
        set(OPENCV_INCLUDE_DIRS "${OPENCV_INCLUDE_DIRS}/opencv4"  CACHE PATH "OpenCV include directory" FORCE)
      endif(EXISTS "${OPENCV_INCLUDE_DIRS}/opencv5")
    endif("${OPENCV_INCLUDE_DIRS}" MATCHES "/include$")

    if(OPENCV_FOUND OR OPENCV_LIBRARIES)
      message(STATUS "OpenCV ${OPENCV_VERSION} found at ${OPENCV_PREFIX}")
      dump(OPENCV_FOUND OPENCV_VERSION OPENCV_INCLUDE_DIRS OPENCV_LIBDIR OPENCV_LINK_FLAGS)

      link_directories(${OPENCV_LIBDIR})
      include_directories(${OPENCV_INCLUDE_DIRS})

      if(OPENCV_LINK_FLAGS)
        set(CMAKE_EXE_LINKER_FLAGS
            "${CMAKE_EXE_LINKER_FLAGS} ${OPENCV_LINK_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS
            "${CMAKE_SHARED_LINKER_FLAGS} ${OPENCV_LINK_FLAGS}")
      endif(OPENCV_LINK_FLAGS)

      set(CMAKE_INSTALL_RPATH "${OPENCV_LIBDIR}:${CMAKE_INSTALL_RPATH}")
      set(CMAKE_BUILD_RPATH "${OPENCV_LIBDIR}:${CMAKE_BUILD_RPATH}")
      set(OPENCV_RESULT TRUE)

      message(STATUS "Finding opencv library - found")
    else(OPENCV_FOUND OR OPENCV_LIBRARIES)
      message(FATAL_ERROR "OpenCV not found. Tried OPENCV_PREFIX='${_opencv_prefix_hint}' "
                          "OpenCV_DIR='${_opencv_dir_hint}' and pkg-config. Pass "
                          "-DOPENCV_PREFIX=<install prefix> or "
                          "-DOpenCV_DIR=<prefix>/lib/cmake/opencv4")
    endif(OPENCV_FOUND OR OPENCV_LIBRARIES)

  endif(NOT OPENCV_FOUND)
  unset(_opencv_prefix_hint)
  unset(_opencv_dir_hint)

  # Snapshot the settled state (post-resolution, so any normalization above
  # is absorbed and won't be mistaken for a fresh user change next time).
  set(_OPENCV_PREFIX_SEEN "${OPENCV_PREFIX}" CACHE INTERNAL
      "last OPENCV_PREFIX find_opencv() resolved against")
  set(_OPENCV_DIR_SEEN "${OpenCV_DIR}" CACHE INTERNAL
      "last OpenCV_DIR find_opencv() resolved against")
endmacro(find_opencv)
