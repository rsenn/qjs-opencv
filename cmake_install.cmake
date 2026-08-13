# Install script for directory: /home/roman/Projects/plot-cv/qjs-opencv

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" "" CMAKE_INSTALL_CONFIG_NAME
                         "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
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

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT
                                                     CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
       "/usr/local/lib/x86_64-linux-gnu/quickjs/libquickjs-opencv.so")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(
      WARNING
        "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}"
    )
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(
      FATAL_ERROR
        "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}"
    )
  endif()
  file(
    INSTALL
    DESTINATION "/usr/local/lib/x86_64-linux-gnu/quickjs"
    TYPE SHARED_LIBRARY PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE FILES
    "/home/roman/Projects/plot-cv/qjs-opencv/libquickjs-opencv.so")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT
                                                     CMAKE_INSTALL_COMPONENT)

endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT
                                                     CMAKE_INSTALL_COMPONENT)
  include(
    "/home/roman/Projects/plot-cv/qjs-opencv/CMakeFiles/quickjs-opencv.dir/install-cxx-module-bmi-noconfig.cmake"
    OPTIONAL)
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/roman/Projects/plot-cv/qjs-opencv/LCCV/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
               "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE
       "/home/roman/Projects/plot-cv/qjs-opencv/install_local_manifest.txt"
       "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
