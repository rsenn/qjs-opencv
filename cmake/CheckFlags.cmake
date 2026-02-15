include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

macro(append_vars STR)
  foreach(L ${ARGN})
    set(LIST "${${L}}")
    if(NOT LIST MATCHES ".*${STR}.*")
      if("${LIST}" STREQUAL "")
        set(LIST "${STR}")
      else("${LIST}" STREQUAL "")
        set(LIST "${LIST} ${STR}")
      endif("${LIST}" STREQUAL "")

    endif(NOT LIST MATCHES ".*${STR}.*")
    string(REPLACE ";" " " LIST "${LIST}")
    # message("New value for ${L}: ${LIST}")
    set("${L}" "${LIST}" PARENT_SCOPE)
  endforeach(L ${ARGN})
endmacro(append_vars STR)

function(check_flag FLAG VAR)
  if(NOT VAR OR VAR STREQUAL "")
    string(TOUPPER "${FLAG}" TMP)
    string(REGEX REPLACE "[^0-9A-Za-z]" _ VAR "${TMP}")
  endif(NOT VAR OR VAR STREQUAL "")
  set(CMAKE_REQUIRED_QUIET ON)
  check_c_compiler_flag("${FLAG}" "${VAR}")
  set(CMAKE_REQUIRED_QUIET OFF)

  set(RESULT "${${VAR}}")
  if(RESULT)
    append_vars(${FLAG} ${ARGN})
    message(STATUS "Compiler flag ${FLAG} ... supported")
  endif(RESULT)
endfunction(check_flag FLAG VAR)

macro(check_flags FLAGS)
  message(STATUS "Checking flags ${FLAGS} ${ARGN}")
  foreach(FLAG ${FLAGS})
    check_flag(${FLAG} "" ${ARGN})
  endforeach(FLAG ${FLAGS})
endmacro(check_flags FLAGS)

macro(check_nowarn_flag FLAG)
  canonicalize(VARNAME "${FLAG}")
  check_c_compiler_flag("${FLAG}" "${VARNAME}")
  #dump(${VARNAME})

  if(${VARNAME})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")

  endif(${VARNAME})
endmacro(check_nowarn_flag FLAG)

macro(ADD_NOWARN_FLAGS)
  string(REGEX REPLACE " -Wall" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  string(REGEX REPLACE " -Wall" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

  nowarn_flag(-Wno-unused-value)
  nowarn_flag(-Wno-unused-variable)

  if("${CMAKE_CXX_COMPILER_ID}" MATCHES ".*Clang.*")
    nowarn_flag(-Wno-deprecated-anon-enum-enum-conversion)
    nowarn_flag(-Wno-extern-c-compat)
    nowarn_flag(-Wno-implicit-int-float-conversion)
    nowarn_flag(-Wno-deprecated-enum-enum-conversion)
  endif("${CMAKE_CXX_COMPILER_ID}" MATCHES ".*Clang.*")

  #dump(CMAKE_C_FLAGS CMAKE_CXX_FLAGS) dump(CMAKE_CXX_FLAGS_DEBUG)
endmacro(ADD_NOWARN_FLAGS)

macro(check_pic_flag)
  if(NOT ARGN)
    set(VAR PIC_FLAG)
  else(NOT ARGN)
    set(VAR "${ARGN}")
  endif(NOT ARGN)
  check_c_compiler_flag("-fPIC" F_PIC)
  if(F_PIC)
    set(${VAR} "${${VAR}} -fPIC")
    set(F_PIC "-fPIC")
  endif(F_PIC)
endmacro(check_pic_flag)

macro(check_opt_none_flag)
  check_c_compiler_flag("-O0" OPT_C_OPT_NONE)
  check_c_compiler_flag("-O0" OPT_CXX_OPT_NONE)
  if(OPT_C_OPT_NONE)
    if(NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-O0")
      set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0"
          CACHE STRING "C compiler options")
    endif(NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-O0")
  endif(OPT_C_OPT_NONE)
  if(OPT_CXX_OPT_NONE)
    if(NOT "${CMAKE_CXX_FLAGS_DEBUG}" MATCHES "-O0")
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0"
          CACHE STRING "C++ compiler options")
    endif(NOT "${CMAKE_CXX_FLAGS_DEBUG}" MATCHES "-O0")
  endif(OPT_CXX_OPT_NONE)
endmacro(check_opt_none_flag)

macro(check_debug_gdb_flag)
  check_c_compiler_flag("-ggdb" OPT_C_G_GDB)
  check_c_compiler_flag("-ggdb" OPT_CXX_G_GDB)
  if(OPT_C_G_GDB)
    if(NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-ggdb")
      set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb"
          CACHE STRING "C compiler options")
    endif(NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-ggdb")
  endif(OPT_C_G_GDB)
  if(OPT_CXX_G_GDB)
    if(NOT "${CMAKE_CXX_FLAGS_DEBUG}" MATCHES "-ggdb")
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb"
          CACHE STRING "C++ compiler options")
    endif(NOT "${CMAKE_CXX_FLAGS_DEBUG}" MATCHES "-ggdb")
  endif(OPT_CXX_G_GDB)
endmacro(check_debug_gdb_flag)

macro(check_c_standard_flag)
  if(NOT C_STANDARD_FLAG)
    foreach(C_STANDARD c2x c11 c99)
      string(REPLACE "c" "C" C_STANDARD_NUM "${C_STANDARD}")
      set(CMAKE_REQUIRED_QUIET TRUE)
      check_c_compiler_flag("-std=${C_STANDARD}" C_STANDARD_${C_STANDARD_NUM})
      set(CMAKE_REQUIRED_QUIET FALSE)
      if(C_STANDARD_${C_STANDARD_NUM})
        string(REGEX REPLACE "\\+" "x" C_STANDARD_NAME "${C_STANDARD_VALUE}")
        string(TOUPPER "${C_STANDARD_NAME}" C_STANDARD_NAME)
        string(REGEX REPLACE "CXX" "" C_STANDARD_VERSION "${C_STANDARD_NAME}")
        #message("C_STANDARD_NAME = ${C_STANDARD_NAME}")
        #message("C_STANDARD_VERSION = ${C_STANDARD_VERSION}")

        if(NOT C_STANDARD_NAME STREQUAL "")
          add_definitions(-D"${C_STANDARD_NAME}")
        endif(NOT C_STANDARD_NAME STREQUAL "")

        set(C_STANDARD_VALUE "${C_STANDARD}" CACHE STRING "C standard")
        set(C_STANDARD_FLAG "-std=${C_STANDARD}" CACHE STRING
                                                       "C standard argument")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_STANDARD_FLAG}")
        break()
      endif(C_STANDARD_${C_STANDARD_NUM})
    endforeach()
  endif(NOT C_STANDARD_FLAG)

  string(REGEX REPLACE "c\\+\\+" "" C_STANDARD_VERSION "${C_STANDARD_VALUE}")
  # add_definitions(-DC_STANDARD=${C_STANDARD_VERSION})

  if(NOT "${CMAKE_C_FLAGS}" MATCHES ".*C_STAND.*")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DC_STANDARD=${C_STANDARD_VERSION}")
  endif(NOT "${CMAKE_C_FLAGS}" MATCHES ".*C_STAND.*")

  message("C standard: ${C_STANDARD_VALUE}")
  message("C flags: ${CMAKE_C_FLAGS}")
endmacro(check_c_standard_flag)

macro(check_cxx_standard_flag)
  foreach(CXX_STANDARD c++2a c++20 c++17 c++14 c++11)
    string(REPLACE "c++" "CPLUSPLUS" CXX_STANDARD_NUM "${CXX_STANDARD}")
    set(CMAKE_REQUIRED_QUIET TRUE)
    #check_cxx_compiler_flag("-std=${CXX_STANDARD}" CXX_STANDARD_${CXX_STANDARD_NUM})

    try_compile(
      CXX_STANDARD_${CXX_STANDARD_NUM} SOURCE_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}" BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}"
      SOURCE_FROM_CONTENT "cxx_standard_${CXX_STANDARD_NUM}.cpp"
      "int main() { return 0; }"
      CMAKE_FLAGS
        "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS} -std=${CXX_STANDARD}"
      COMPILE_DEFINITIONS "-std=${CXX_STANDARD}" LOG_DESCRIPTION
                          "cxx_standard_${CXX_STANDARD_NUM}"
      OUTPUT_VARIABLE OUT)

    if(NOT CXX_STANDARD_${CXX_STANDARD_NUM})
      message(
        STATUS "Checking for C++ standard flag: -std=${CXX_STANDARD}: failed")
      continue()
    endif(NOT CXX_STANDARD_${CXX_STANDARD_NUM})

    set(CMAKE_REQUIRED_QUIET FALSE)
    if(CXX_STANDARD_${CXX_STANDARD_NUM})
      string(REGEX REPLACE "\\+" "x" CXX_STANDARD_NAME "${CXX_STANDARD_VALUE}")
      string(TOUPPER "${CXX_STANDARD_NAME}" CXX_STANDARD_NAME)
      string(REGEX REPLACE "CXX" "" CXX_STANDARD_VERSION "${CXX_STANDARD_NAME}")

      if(NOT CXX_STANDARD_NAME STREQUAL "")
        add_definitions(-D"${CXX_STANDARD_NAME}")
      endif(NOT CXX_STANDARD_NAME STREQUAL "")

      set(CXX_STANDARD_VALUE "${CXX_STANDARD}" CACHE STRING "C++ standard"
                                                     FORCE)
      set(CXX_STANDARD_FLAG "-std=${CXX_STANDARD}"
          CACHE STRING "C++ standard argument" FORCE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_STANDARD_FLAG}")
      message(STATUS "Checking for C++ standard flag: -std=${CXX_STANDARD}: OK")
      break()
    endif(CXX_STANDARD_${CXX_STANDARD_NUM})
  endforeach()

  string(REGEX REPLACE "c\\+\\+" "" CXX_STANDARD_VERSION
                       "${CXX_STANDARD_VALUE}")
  # add_definitions(-DCXX_STANDARD=${CXX_STANDARD_VERSION})

  if(NOT "${CMAKE_CXX_FLAGS}" MATCHES ".*CXX_STAND.*")
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -DCXX_STANDARD=${CXX_STANDARD_VERSION}")
  endif(NOT "${CMAKE_CXX_FLAGS}" MATCHES ".*CXX_STAND.*")

  message("C++ standard: ${CXX_STANDARD_VALUE}")
  message("C++ flags: ${CMAKE_CXX_FLAGS}")
endmacro(check_cxx_standard_flag)

macro(check_nowarn_flag FLAG)
  canonicalize(VARNAME "${FLAG}")
  check_c_compiler_flag("${FLAG}" "${VARNAME}")
  #dump(${VARNAME})

  if(${VARNAME})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}")

  endif(${VARNAME})
endmacro(check_nowarn_flag FLAG)
