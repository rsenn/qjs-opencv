#include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Functions.cmake)

macro(find_opencv)
  function(OPENCV_CHANGE_DIR VAR ACCESS VALUE LIST_FILE STACK)
    string(REGEX REPLACE "/lib/.*" "" OPENCV_ROOT "${VALUE}")
    set(OPENCV_PREFIX "${OPENCV_ROOT}" PARENT_SCOPE)
  endfunction(OPENCV_CHANGE_DIR VAR ACCESS VALUE LIST_FILE STACK)

  function(OPENCV_CHANGE VAR ACCESS VALUE LIST_FILE STACK)

    # message("VAR ${VAR} changed!!!")
    unset(OPENCV_FOUND CACHE)
    unset(OPENCV_FOUND PARENT_SCOPE)
    unset(OPENCV_CHECKED CACHE)
    unset(OPENCV_CHECKED PARENT_SCOPE)

    unset_all(
      OPENCV_CFLAGS
      OPENCV_INCLUDEDIR
      OPENCV_INCLUDE_DIRS
      OPENCV_LDFLAGS
      OPENCV_LIBDIR
      OPENCV_LIBRARIES
      OPENCV_LIBRARY_DIRS
      OPENCV_MODULE_NAME
      OPENCV_PREFIX
      OPENCV_ROOT
      OPENCV_STATIC_CFLAGS
      OPENCV_STATIC_INCLUDE_DIRS
      OPENCV_STATIC_LDFLAGS
      OPENCV_STATIC_LIBRARIES
      OPENCV_STATIC_LIBRARY_DIRS
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

  endfunction(OPENCV_CHANGE VAR ACCESS VALUE LIST_FILE STACK)

  variable_watch(OpenCV_DIR OPENCV_CHANGE_DIR)
  variable_watch(OpenCV_DIR OPENCV_CHANGE)

  if(NOT OPENCV_PREFIX)
    if(OpenCV_DIR)
      set(OPENCV_PREFIX "${OpenCV_DIR}")
    endif(OpenCV_DIR)
  endif(NOT OPENCV_PREFIX)

  # dump(OPENCV_PREFIX)

  if(NOT OPENCV_CHECKED)
    message(CHECK_START "Finding opencv library")

    set(OPENCV_PREFIX "${OPENCV_PREFIX}" CACHE PATH "OpenCV root dir")

    if(OPENCV_PREFIX)
      list(APPEND CMAKE_PREFIX_PATH "${OPENCV_PREFIX}")
      list(APPEND CMAKE_MODULE_PATH "${OPENCV_PREFIX}/lib/cmake/opencv4")
    endif(OPENCV_PREFIX)

    # dump(CMAKE_PREFIX_PATH CMAKE_MODULE_PATH)

    if(NOT OPENCV_FOUND)
      find_package(OpenCV PATHS "${OPENCV_PREFIX}/lib/cmake/opencv4;${OPENCV_PREFIX}/lib/cmake;${OPENCV_PREFIX}")
      # message(STATUS "OpenCV_VERSION = ${OpenCV_VERSION}")
      if(OpenCV_VERSION)

        set(OPENCV_VERSION "${OpenCV_VERSION}" CACHE PATH "OpenCV version")
        set(OPENCV_LIBDIR "${OpenCV_INSTALL_PATH}/lib" CACHE PATH "OpenCV library directory")
        set(OPENCV_LINK_FLAGS "-Wl,-rpath,${OPENCV_LIBDIR} -L${OPENCV_LIBDIR}" CACHE STRING "OpenCV link flags")
        set(OPENCV_PREFIX "${OpenCV_INSTALL_PATH}" CACHE PATH "OpenCV install directory")
        set(OPENCV_INCLUDE_DIRS "${OpenCV_INCLUDE_DIRS}" CACHE PATH "OpenCV include directories")
        set(OPENCV_LIBRARIES "${OpenCV_LIBS}" CACHE PATH "OpenCV libraries")

        # dump(OpenCV_LIBS OpenCV_INCLUDE_DIRS OpenCV_VERSION OpenCV_SHARED OpenCV_INSTALL_PATH OpenCV_LIB_COMPONENTS) dump(OPENCV_PREFIX OPENCV_LIBDIR OPENCV_LINK_FLAGS OPENCV_INCLUDE_DIRS OPENCV_LIBRARIES)
        set(OPENCV_FOUND TRUE)
      endif(OpenCV_VERSION)
    endif(NOT OPENCV_FOUND)

    if(NOT OPENCV_FOUND)
      set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
      pkg_search_module(OPENCV REQUIRED opencv opencv4)

      if(OPENCV_FOUND)
        message("OpenCV with pkg_search_module")
      endif(OPENCV_FOUND)
    endif(NOT OPENCV_FOUND)

    if(OPENCV_FOUND OR OPENCV_LIBRARIES)

      link_directories(${OPENCV_LIB_DIR})
      include_directories(${OPENCV_INCLUDE_DIRS})

      if(OPENCV_LINK_FLAGS)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OPENCV_LINK_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${OPENCV_LINK_FLAGS}")
      endif(OPENCV_LINK_FLAGS)

      set(CMAKE_INSTALL_RPATH "${OPENCV_LIB_DIR}:${CMAKE_INSTALL_RPATH}")
      set(CMAKE_BUILD_RPATH "${OPENCV_LIB_DIR}:${CMAKE_BUILD_RPATH}")
      set(OPENCV_RESULT TRUE)
      message(CHECK_PASS "found")

    else(OPENCV_FOUND OR OPENCV_LIBRARIES)
      message(CHECK_FAIL "fail")
    endif(OPENCV_FOUND OR OPENCV_LIBRARIES)

    if(OPENCV_FOUND)
      message("OpenCV found: ${OPENCV_PREFIX}")
      # set(OPENCV_PREFIX "${OPENCV_PREFIX}" CACHE PATH "OpenCV install prefix")
    endif(OPENCV_FOUND)
    set(OPENCV_CHECKED TRUE)

    # dump(OPENCV_FOUND OPENCV_INCLUDE_DIRS OPENCV_LIBDIR OPENCV_LINK_FLAGS OPENCV_LIBRARIES)
  endif(NOT OPENCV_CHECKED)
endmacro(find_opencv)
