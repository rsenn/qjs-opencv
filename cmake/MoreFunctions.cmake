include(CheckLibraryExists)
include(CheckTypeSize)

function(DUMP VAR)
  string(REGEX REPLACE "[;\n]" " " A "${ARGV}")
  foreach(VAR ${ARGV})
    message("  ${VAR} = ${${VAR}}")
  endforeach(VAR ${ARGV})
endfunction(DUMP VAR)

function(CANONICALIZE OUTPUT_VAR STR)
  string(REGEX REPLACE "^-W" "WARN_" TMP_STR "${STR}")

  string(REGEX REPLACE "-" "_" TMP_STR "${TMP_STR}")
  string(TOUPPER "${TMP_STR}" TMP_STR)

  set("${OUTPUT_VAR}" "${TMP_STR}" PARENT_SCOPE)
endfunction(CANONICALIZE OUTPUT_VAR STR)

macro(unset_all)
  foreach(VAR ${ARGN})
    unset("${VAR}" PARENT_SCOPE)
    unset("${VAR}" CACHE)
  endforeach(VAR ${ARGN})
endmacro(unset_all)

macro(check_size TYPE VAR)
  check_type_size("${TYPE}" CMAKE_${VAR})
  message(STATUS "size of ${TYPE} ${CMAKE_${VAR}}")
endmacro(check_size TYPE VAR)

macro(find_static_library VAR NAME)
  find_library(${VAR} NAMES lib${NAME}.a PATHS "${CMAKE_INSTALL_PREFIX}/lib" NO_DEFAULT_PATH)
endmacro()

function(ADD_UNIQUE LIST)
  set(RESULT "${${LIST}}")
  foreach(ITEM ${ARGN})
    contains(RESULT "${ITEM}" FOUND)
    if(NOT FOUND)
      list(APPEND RESULT "${ITEM}")
    endif(NOT FOUND)
  endforeach(ITEM ${ARGN})
  set("${LIST}" "${RESULT}" PARENT_SCOPE)
endfunction(ADD_UNIQUE LIST)

function(CONTAINS LIST VALUE OUTPUT)
  list(FIND "${LIST}" "${VALUE}" INDEX)
  if(${INDEX} GREATER -1)
    set(RESULT TRUE)
  else(${INDEX} GREATER -1)
    set(RESULT FALSE)
  endif(${INDEX} GREATER -1)
  if(NOT RESULT)
    foreach(ITEM ${${LIST}})
      if("${ITEM}" STREQUAL "${VALUE}")
        set(RESULT TRUE)
      endif("${ITEM}" STREQUAL "${VALUE}")
    endforeach(ITEM ${${LIST}})
  endif(NOT RESULT)
  set("${OUTPUT}" "${RESULT}" PARENT_SCOPE)
endfunction(CONTAINS LIST VALUE OUTPUT)
