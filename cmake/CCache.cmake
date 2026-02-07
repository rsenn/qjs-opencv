set(__FIND_ROOT_PATH_MODE_PROGRAM ${CMAKE_FIND_ROOT_PATH_MODE_PROGRAM})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

find_program(CCACHE_EXECUTABLE ccache QUIET)
mark_as_advanced(CCACHE_EXECUTABLE)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ${__FIND_ROOT_PATH_MODE_PROGRAM})
set(__FIND_ROOT_PATH_MODE_PROGRAM)

macro(use_ccache)
  if(NOT CCACHE_EXECUTABLE)
    message(FATAL_ERROR "use_ccache: ccache not found.")
  endif()

  if(CMAKE_VERSION VERSION_LESS 3.4)
    # Prior to 3.4, had to use these properties Note: These may conflict with CTest
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_EXECUTABLE}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_EXECUTABLE}")
  else()
    # CMake 3.4 introduced 'COMPILER_LAUNCHER'
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_EXECUTABLE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXECUTABLE}")
  endif()
endmacro()

function(target_enable_ccache target)
  if(NOT CCACHE_EXECUTABLE)
    message(FATAL_ERROR "target_enable_ccache: ccache not found.")
  endif()

  if(CMAKE_VERSION VERSION_GREATER 3.3)
    # CMake 3.4 introduced 'COMPILER_LAUNCHER'
    set_property(TARGET "${target}" PROPERTY C_COMPILER_LAUNCHER
                                             "${CCACHE_EXECUTABLE}")
    set_property(TARGET "${target}" PROPERTY CXX_COMPILER_LAUNCHER
                                             "${CCACHE_EXECUTABLE}")
  else()
    # Prior to 3.4, had to use these properties Note: These may conflict with CTest
    set_property(TARGET "${target}" PROPERTY RULE_LAUNCH_COMPILE
                                             "${CCACHE_EXECUTABLE}")
    set_property(TARGET "${target}" PROPERTY RULE_LAUNCH_LINK
                                             "${CCACHE_EXECUTABLE}")
  endif()
endfunction()

if(CCACHE_EXECUTABLE)
  option(ENABLE_CCACHE "Enable compiler cache" OFF)
endif(CCACHE_EXECUTABLE)

if(ENABLE_CCACHE)
  use_ccache()
endif(ENABLE_CCACHE)
