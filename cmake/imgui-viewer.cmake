#[[cmake_minimum_required(VERSION 3.9)
project(imgui-viewer)
set(CMAKE_CXX_STANDARD 14)
]]

include(${CMAKE_SOURCE_DIR}/cmake/FindGLFW.cmake)
# include: OpenCV
include(${CMAKE_SOURCE_DIR}/cmake/opencv.cmake)

# glfw
include(FindPkgConfig)
pkg_check_modules(GLFW3 glfw3 REQUIRED)
#[[

find_package(glfw3 REQUIRED)]]
include_directories(${GLFW_INCLUDE_DIRS})
link_libraries(${GLFW_LIBRARY_DIRS})

set(GLFW_LIBRARIES ${GLFW_LIBRARY} ${pkgcfg_lib_GLFW3_glfw})
# opengl
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

# glew
pkg_check_modules(GLEW glew REQUIRED)

set(GLEW_LIBRARIES ${pkgcfg_lib_GLEW_GL} ${pkgcfg_lib_GLEW_GLEW}
                   ${pkgcfg_lib_GLEW_GLU})
# find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(OpenGL_LIBRARY OpenGL)
  find_library(IOKIT_LIBRARY IOKit)
  find_library(COREVIDEO_LIBRARY CoreVideo)
  set(EXTRA_LIBS ${COCOA_LIBRARY} ${OpenGL_LIBRARY} ${IOKIT_LIBRARY}
                 ${COREVIDEO_LIBRARY})
endif(APPLE)

if(WIN32)
  # nothing now
endif(WIN32)
include(${CMAKE_SOURCE_DIR}/cmake/sdl2-config.cmake)

if(ANDROID)
  add_definitions(-DIMGUI_IMPL_OPENGL_ES2=1)
else(ANDROID)
  find_package(GLEW)

  add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLEW=1 -DIMGUI_DEBUG_LOG=1
                  -DIMGUI_IMPL_OPENGL_LOADER_GLEW=1)
endif(ANDROID)

file(
  GLOB
  IMGUI_VIEWER_SOURCES
  color.cpp
  data.cpp
  geometry.cpp
  js.cpp
  jsbindings.cpp
  plot-cv.cpp
  js_*.cpp
  line.cpp
  matrix.cpp
  polygon.cpp
  util.cpp
  *.h
  *.hpp)

set(QUICKJS_SOURCES ${quickjs_sources})

add_definitions(-D_GNU_SOURCE=1)

# Main
add_executable(
  imgui-viewer
  imgui-viewer.cpp
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_widgets.cpp
  imgui/imgui_impl_sdl.cpp
  imgui/imgui_impl_opengl3.cpp
  imgui/libs/gl3w/GL/gl3w.c
  ${IMGUI_VIEWER_SOURCES})
target_compile_definitions(
  imgui-viewer
  PRIVATE CONFIG_VERSION="${quickjs_version}"
          CONFIG_PREFIX="${CMAKE_INSTALL_PREFIX}" CONFIG_BIGNUM=1
          ${PLOTCV_DEFS})

# link
target_link_libraries(
  imgui-viewer
  ${SDL2_LIBRARIES}
  ${OpenCV_LIBS}
  glfw
  ${OPENGL_LIBRARIES}
  ${GLFW_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${EXTRA_LIBS}
  quickjs
  png
  ${LIBDL}
  ${LIBM}
  ${LIBPTHREAD}
  GL)
if(OpenCV_FOUND)
  target_include_directories(imgui-viewer PUBLIC ${OpenCV_INCLUDE_DIRS})
  target_link_libraries(imgui-viewer ${OpenCV_LIBS})
endif()

install(TARGETS imgui-viewer DESTINATION bin)
