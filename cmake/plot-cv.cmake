file(
  GLOB
  PLOT_CV_SOURCES
  color.cpp
  data.cpp
  geometry.cpp
  js.cpp
  jsbindings.cpp
  # plot-cv.cpp
  js_contour.cpp
  js_draw.cpp
  js_mat.cpp
  js_point.cpp
  js_rect.cpp
  js_size.cpp
  line.cpp
  matrix.cpp
  polygon.cpp
  *.h
  *.hpp)

add_executable(plot-cv plot-cv.cpp ${PLOT_CV_SOURCES})
target_link_libraries(plot-cv ${OpenCV_LIBS} quickjs-static
                      ${ELECTRICFENCE_LIBRARY})
