include(${CMAKE_SOURCE_DIR}/cmake/opencv.cmake)

set(GLEW_USE_STATIC_LIBS TRUE)
set(GLEW_VERBOSE TRUE)

file(GLOB HIGHGUI_VIEWER_SOURCES color.cpp data.cpp geometry.cpp js.cpp jsbindings.cpp plot-cv.cpp js_*.cpp line.cpp matrix.cpp polygon.cpp util.cpp *.h *.hpp)

set(QUICKJS_SOURCES ${quickjs_sources})

add_definitions(-D_GNU_SOURCE=1)

add_executable(highgui-viewer highgui-viewer.cpp ${HIGHGUI_VIEWER_SOURCES} )
target_compile_definitions(highgui-viewer PRIVATE _GNU_SOURCE=1 CONFIG_VERSION="${quickjs_version}" CONFIG_PREFIX="${CMAKE_INSTALL_PREFIX}" CONFIG_BIGNUM=1 ${PLOTCV_DEFS})

target_link_libraries(
  highgui-viewer ${OpenCV_LIBS} ${GLEW_SHARED_LIBRARY_RELEASE} ${GLEW_SHARED_LIBRARIES}
  quickjs
  png
  ${LIBDL} ${LIBM} ${LIBPTHREAD})

install(TARGETS highgui-viewer DESTINATION bin)
