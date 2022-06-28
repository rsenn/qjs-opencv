
add_definitions(-D_FILE_OFFSET_BITS=64)
if(CMAKE_COMPILER_IS_GNUCXX)
  add_compile_options(-Wno-psabi)
endif()

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBCAMERA REQUIRED libcamera)

include_directories(${LIBCAMERA_INCLUDE_DIRS})

file(GLOB LIBCAMERA_OPENCV_SOURCES CONFIGURE_DEPENDS libcamera-opencv/*.h libcamera-opencv/*.cpp)
list(FILTER LIBCAMERA_OPENCV_SOURCES EXCLUDE REGEX main.cpp)

add_library(camera-opencv ${LIBCAMERA_OPENCV_SOURCES})
target_link_libraries(camera-opencv stdc++fs camera camera-base event event_pthreads Threads::Threads ${OpenCV_LIBS})
