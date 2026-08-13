# Install script for directory: /mnt/data/Projects/plot-cv/qjs-opencv

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/usr/local/lib/x86_64-linux-gnu/quickjs/opencv.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/lib/x86_64-linux-gnu/quickjs/opencv.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/local/lib/x86_64-linux-gnu/quickjs/opencv.so"
         RPATH "/mnt/data/opt/opencv-5.0.0-x86_64/lib")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/lib/x86_64-linux-gnu/quickjs/opencv.so")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/lib/x86_64-linux-gnu/quickjs" TYPE SHARED_LIBRARY PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES "/mnt/data/Projects/plot-cv/qjs-opencv/build/x86_64-linux-debug/opencv.so")
  if(EXISTS "$ENV{DESTDIR}/usr/local/lib/x86_64-linux-gnu/quickjs/opencv.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/lib/x86_64-linux-gnu/quickjs/opencv.so")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}/usr/local/lib/x86_64-linux-gnu/quickjs/opencv.so"
         OLD_RPATH "/mnt/data/Projects/plot-cv/qjs-opencv/build/x86_64-linux-debug:/mnt/data/Projects/plot-cv/qjs-opencv/build/x86_64-linux-debug/quickjs:/mnt/data/Projects/plot-cv/qjs-opencv/build/x86_64-linux-debug/quickjs:/usr/local/lib/x86_64-linux-gnu/quickjs:/mnt/data/opt/opencv-5.0.0-x86_64/lib:/usr/local/include/quickjs:/usr/local/lib/x86_64-linux-gnu:/mnt/data/Projects/plot-cv/qjs-opencv/build/x86_64-linux-debug/quickjs:/mnt/data/Projects/plot-cv/qjs-opencv/build/x86_64-linux-debug/LCCV:/opt/opencv-5.0.0-x86_64/lib:"
         NEW_RPATH "/mnt/data/opt/opencv-5.0.0-x86_64/lib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/local/lib/x86_64-linux-gnu/quickjs/opencv.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  include("/mnt/data/Projects/plot-cv/qjs-opencv/build/x86_64-linux-debug/CMakeFiles/quickjs-opencv.dir/install-cxx-module-bmi-Debug.cmake" OPTIONAL)
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/mnt/data/Projects/plot-cv/qjs-opencv/build/x86_64-linux-debug/LCCV/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/mnt/data/Projects/plot-cv/qjs-opencv/build/x86_64-linux-debug/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/mnt/data/Projects/plot-cv/qjs-opencv/build/x86_64-linux-debug/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
