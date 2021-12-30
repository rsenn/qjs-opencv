if(WIN32 OR MINGW)
  set(QUICKJS_LIBRARY_DIR "${QUICKJS_PREFIX}/bin")
  set(QUICKJS_MODULE_DEPENDENCIES quickjs)
endif(WIN32 OR MINGW)

set(QUICKJS_MODULE_CFLAGS "-fvisibility=hidden")

function(config_shared_module TARGET_NAME)
  if(QUICKJS_LIBRARY_DIR)
    link_directories("${QUICKJS_LIBRARY_DIR}")
  endif(QUICKJS_LIBRARY_DIR)
  if(QUICKJS_MODULE_DEPENDENCIES)
    target_link_libraries(${TARGET_NAME} ${QUICKJS_MODULE_DEPENDENCIES})
  endif(QUICKJS_MODULE_DEPENDENCIES)
  if(QUICKJS_MODULE_CFLAGS)
    target_compile_options(${TARGET_NAME} PRIVATE "${QUICKJS_MODULE_CFLAGS}")
  endif(QUICKJS_MODULE_CFLAGS)
endfunction(config_shared_module TARGET_NAME)

set(JS_BINDINGS_COMMON color.hpp geometry.hpp js.hpp js_alloc.hpp js_array.hpp js_contour.hpp js_line.hpp js_point.hpp
                       js_rect.hpp js_size.hpp js_typed_array.hpp jsbindings.hpp psimpl.hpp util.hpp)
set(js_line_SOURCES line.cpp line.hpp)

function(make_shared_module FNAME)
  message("make_shared_module(${FNAME})")
  string(REGEX REPLACE "_" "-" NAME "${FNAME}")
  string(TOUPPER "${FNAME}" UNAME)

  set(TARGET_NAME quickjs-${NAME})

  if(ARGN)
    set(SOURCES ${ARGN})
  else(ARGN)
    set(SOURCES js_${FNAME}.cpp ${js_${FNAME}_SOURCES} jsbindings.cpp util.cpp js.hpp js.cpp ${JS_BINDINGS_COMMON})
  endif(ARGN)

  add_library(${TARGET_NAME} SHARED ${SOURCES})

  target_link_libraries(${TARGET_NAME} ${jsbindings_LIBRARIES} ${OpenCV_LIBS})
  set_target_properties(
    ${TARGET_NAME}
    PROPERTIES
      PREFIX "" # BUILD_RPATH "${OPENCV_LIBRARY_DIRS}:${CMAKE_CURRENT_BINARY_DIR}"
      RPATH "${OPENCV_LIBRARY_DIRS}:${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/lib/quickjs"
      OUTPUT_NAME "${NAME}"
      COMPILE_FLAGS "${QUICKJS_MODULE_CFLAGS}"
      BUILD_RPATH
      "${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_CURRENT_BINARY_DIR}/quickjs:${CMAKE_CURRENT_BINARY_DIR}/quickjs"
  )
  target_compile_definitions(${TARGET_NAME} PRIVATE JS_${UNAME}_MODULE CONFIG_PREFIX="${CMAKE_INSTALL_PREFIX}")
  install(TARGETS ${TARGET_NAME} DESTINATION lib/quickjs PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
                                                                     GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

  config_shared_module(${TARGET_NAME})

  if(OpenCV_FOUND)
    target_include_directories(${TARGET_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${OpenCV_LIBS})
  endif()
endfunction()

function(make_static_module FNAME)
  message("make_static_module(${FNAME})")
  string(REGEX REPLACE "_" "-" NAME "${FNAME}")
  string(TOUPPER "${FNAME}" UNAME)

  set(TARGET_NAME quickjs-${NAME}-static)

  if(ARGN)
    set(SOURCES ${ARGN})
  else(ARGN)
    set(SOURCES js_${FNAME}.cpp ${js_${FNAME}_SOURCES} jsbindings.cpp util.cpp js.hpp js.cpp ${JS_BINDINGS_COMMON})
  endif(ARGN)

  add_library(${TARGET_NAME} STATIC ${SOURCES})

  target_link_libraries(${TARGET_NAME} ${jsbindings_LIBRARIES} ${OpenCV_LIBS})
  set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "" OUTPUT_NAME "${NAME}" COMPILE_FLAGS
                                                                                  "${QUICKJS_MODULE_CFLAGS}")
  target_compile_definitions(${TARGET_NAME} PRIVATE CONFIG_PREFIX="${CMAKE_INSTALL_PREFIX}")
  install(TARGETS ${TARGET_NAME} DESTINATION lib/quickjs)

  config_shared_module(${TARGET_NAME})

  if(OpenCV_FOUND)
    target_include_directories(${TARGET_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} ${OpenCV_LIBS})
  endif()
endfunction()

function(make_js_bindings)
  message("make_js_bindings")
  file(GLOB JS_BINDINGS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/js_*.cpp)

  foreach(MOD ${JS_BINDINGS_SOURCES})
    string(REGEX REPLACE "\\.cpp" "" MOD "${MOD}")
    string(REGEX REPLACE ".*/js_" "" MOD "${MOD}")
    list(APPEND JS_BINDINGS_MODULES ${MOD})
  endforeach(MOD ${JS_BINDINGS_SOURCES})

  foreach(JS_MODULE ${JS_BINDINGS_MODULES})
    make_shared_module(${JS_MODULE})
  endforeach()

  string(REPLACE ";" " " MODULE_NAMES "${JS_BINDINGS_MODULES}")
  message(STATUS "Configured modules: ${MODULE_NAMES}")

  add_dependencies(quickjs-rect quickjs-point quickjs-size)
  # add_dependencies(quickjs-contour quickjs-mat quickjs-rect quickjs-point)

  add_dependencies(quickjs-contour quickjs-mat)

  target_link_libraries(quickjs-mat quickjs-size)
  target_link_libraries(quickjs-point-iterator quickjs-line quickjs-point)
  target_link_libraries(quickjs-contour quickjs-point-iterator quickjs-mat)
  target_link_libraries(quickjs-line quickjs-point)
  target_link_libraries(quickjs-rect quickjs-size quickjs-point)
  target_link_libraries(quickjs-video-capture quickjs-mat)
  target_link_libraries(quickjs-cv quickjs-mat quickjs-contour quickjs-rect quickjs-line)
  target_link_libraries(quickjs-draw quickjs-mat quickjs-contour quickjs-size)
  target_link_libraries(quickjs-clahe quickjs-mat quickjs-size)
  target_link_libraries(quickjs-umat quickjs-mat)
  target_link_libraries(quickjs-subdiv2d quickjs-contour)

  target_link_libraries(quickjs-cv png)

  # add_dependencies(quickjs-point-iterator quickjs-contour quickjs-mat)

  file(GLOB JS_BINDINGS_SOURCES color.cpp data.cpp geometry.cpp jsbindings.cpp js_*.cpp js.cpp line.cpp matrix.cpp
       polygon.cpp *.h *.hpp)

  # Main
  add_library(quickjs-opencv MODULE ${JS_BINDINGS_SOURCES})
  config_shared_module(quickjs-opencv)

  set_target_properties(
    quickjs-opencv
    PROPERTIES # COMPILE_FLAGS "-fvisibility=hidden"
               RPATH "${OPENCV_LIBRARY_DIRS}:${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_PREFIX}/lib/quickjs"
               OUTPUT_NAME "opencv" PREFIX "")
  target_compile_definitions(quickjs-opencv PRIVATE -DCONFIG_PREFIX=\"${CMAKE_INSTALL_PREFIX}\")

  target_link_libraries(quickjs-opencv ${OpenCV_LIBS})
  # link
endfunction(make_js_bindings)
