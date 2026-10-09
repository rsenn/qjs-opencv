include(CheckLibraryExists)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)

#
# add_cflags <ADD> [OUTPUT_VAR]
#
# Append the flag ADD to OUTPUT_VAR (CMAKE_C_FLAGS by default) unless it is
# already there.
#
function(add_cflags ADD)
  if(${ARGC} LESS 2)
    set(OUTPUT_VAR CMAKE_C_FLAGS)
  else()
    set(OUTPUT_VAR "${ARGV1}")
  endif()

  set(RESULT "${${OUTPUT_VAR}}")
  string(REGEX REPLACE " +" ";" FLAGS "${RESULT}")
  string(REGEX REPLACE "^;+" "" FLAGS "${FLAGS}")
  list(REMOVE_DUPLICATES FLAGS)
  if(NOT ADD IN_LIST FLAGS)
    list(APPEND RESULT ${ADD})
  endif()

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# set_init <OUTPUT-VAR> [ITEMS...]
#
# Store the unique ITEMS, sorted, in OUTPUT-VAR.
#
function(set_init OUTPUT_VAR)
  set(RESULT "")
  set_add(RESULT ${ARGN})
  list(SORT RESULT)
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# set_add <OUTPUT-VAR> [ITEMS...]
#
# Append the ITEMS not already present to the set in OUTPUT-VAR.
#
function(set_add OUTPUT_VAR)
  set(RESULT "${${OUTPUT_VAR}}")
  foreach(ITEM ${ARGN})
    if(NOT ITEM IN_LIST RESULT)
      list(APPEND RESULT "${ITEM}")
    endif()
  endforeach()

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# set_symmetric_difference <NOT-IN-A> <NOT-IN-B> <SET-A> <SET_B>
#
# Store the items only in SET-B in NOT-IN-A and the items only in SET-A in
# NOT-IN-B.
#
function(set_symmetric_difference NOT_IN_A NOT_IN_B SET_A SET_B)
  set(B "")
  foreach(ITEM ${${SET_A}})
    if(NOT ITEM IN_LIST "${SET_B}")
      list(APPEND B "${ITEM}")
    endif()
  endforeach()

  set(A "")
    foreach(ITEM ${${SET_B}})
    if(NOT ITEM IN_LIST "${SET_A}")
      list(APPEND A "${ITEM}")
    endif()
  endforeach()

  set("${NOT_IN_A}" "${A}" PARENT_SCOPE)
  set("${NOT_IN_B}" "${B}" PARENT_SCOPE)
endfunction()

#
# set_difference <OUTPUT-VAR> <SET-A> <SET_B>
#
# Store the items of SET-A that are not in SET-B in OUTPUT-VAR.
#
function(set_difference OUTPUT_VAR SET_A SET_B)
  set(OUT "")
  foreach(ITEM ${${SET_A}})
    if(NOT ITEM IN_LIST "${SET_B}")
      list(APPEND OUT "${ITEM}")
    endif()
  endforeach()

  set("${OUTPUT_VAR}" "${OUT}" PARENT_SCOPE)
endfunction()

#
# set_intersection <OUTPUT-VAR> <SET-A> <SET_B>
#
# Store the items of SET-A that are also in SET-B in OUTPUT-VAR.
#
function(set_intersection OUTPUT_VAR SET_A SET_B)
  set(OUT "")
  foreach(ITEM ${${SET_A}})
    if(ITEM IN_LIST "${SET_B}")
      list(APPEND OUT "${ITEM}")
    endif()
  endforeach()

  set("${OUTPUT_VAR}" "${OUT}" PARENT_SCOPE)
endfunction()

#
# set_union <OUTPUT-VAR> <SET-A> <SET_B>
#
# Store the items that are in SET-A or in SET-B - without duplicates -
# in OUTPUT-VAR.
#
function(set_union OUTPUT_VAR SET_A SET_B)
  set(OUT "")
  foreach(ITEM ${${SET_A}} ${${SET_B}})
    if(NOT ITEM IN_LIST OUT)
      list(APPEND OUT "${ITEM}")
    endif()
  endforeach()

  set("${OUTPUT_VAR}" "${OUT}" PARENT_SCOPE)
endfunction()

#
# set_retain <SET-VAR> [ARGS...]
#
# Reduce SET-VAR to those ARGS that are already in it.
#
function(set_retain SET_VAR)
  set(OUT "")
  foreach(ITEM ${ARGN})
    if(ITEM IN_LIST "${SET_VAR}")
      list(APPEND OUT "${ITEM}")
    endif()
  endforeach()

  set("${SET_VAR}" "${OUT}" PARENT_SCOPE)
endfunction()

#
# set_missing <OUTPUT-VAR> <SET-VAR> [ARGS...]
#
# Store the ARGS that are not in SET-VAR in OUTPUT-VAR.
#
function(set_missing OUTPUT_VAR SET_VAR)
  set(OUT "")
  foreach(ITEM ${ARGN})
    if(NOT ITEM IN_LIST "${SET_VAR}")
      list(APPEND OUT "${ITEM}")
    endif()
  endforeach()

  set("${OUTPUT_VAR}" "${OUT}" PARENT_SCOPE)
endfunction()

#
# set_extra <OUTPUT-VAR> <SET-VAR> [ARGS...]
#
# Store the items of SET-VAR that are not in ARGS in OUTPUT-VAR.
#
function(set_extra OUTPUT_VAR SET_VAR)
  set(OUT "")
  foreach(ITEM ${${SET_VAR}})
    if(NOT ITEM IN_LIST ARGN)
      list(APPEND OUT "${ITEM}")
    endif()
  endforeach()

  set("${OUTPUT_VAR}" "${OUT}" PARENT_SCOPE)
endfunction()

#
# set_remove <SET-VAR> [ARGS...]
#
# Remove the ARGS from SET-VAR.
#
function(set_remove SET_VAR)
  set(OUT "${${SET_VAR}}")
  list(REMOVE_ITEM OUT ${ARGN})
  set("${SET_VAR}" "${OUT}" PARENT_SCOPE)
endfunction()

#
# set_transform <SET-VAR> <FUNC> [FUNC-ARGS...]
#
# Call FUNC([FUNC-ARGS...] ITEM <item>) for every item of SET-VAR.
#
function(set_transform SET_VAR FUNC)
  set(OUT "")
  set(ARGS "ITEM \"\${ITEM}\"")
  if(ARGN)
    list(JOIN ARGN " " PREPEND)
    set(ARGS "${PREPEND} ${ARGS}")
  endif()

  foreach(ITEM ${${SET_VAR}})
    cmake_language(EVAL CODE "${FUNC}(${ARGS})")
    list(APPEND OUT "${ITEM}")
  endforeach()

  set("${SET_VAR}" "${OUT}" PARENT_SCOPE)
endfunction()

#
# escape_string <OUTPUT-VAR> <STR>
#
# Store STR in OUTPUT-VAR with backslash and control characters (newline, CR,
# tab, ESC) written as escape sequences.
#
function(escape_string OUTPUT_VAR STR)
  string(REPLACE "\\" "\\\\" RESULT "${STR}")
  string(REPLACE "\n" "\\n" RESULT "${RESULT}")
  string(REPLACE "\r" "\\r" RESULT "${RESULT}")
  string(REPLACE "\t" "\\t" RESULT "${RESULT}")
  string(REPLACE "${ANSI_ESCAPE}" "\\e" RESULT "${RESULT}")
  string(REPLACE "" "\\v" RESULT "${RESULT}")
  string(REPLACE "" "\\f" RESULT "${RESULT}")
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif(OUTPUT_VAR)
endfunction()

#
# unescape_string <OUTPUT-VAR> <STR>
#
# Store STR in OUTPUT-VAR with backslash escape sequences (\n, \r, \t, \e,
# \x1b, \033) turned back into characters.
#
function(unescape_string OUTPUT_VAR STR)
  string(REPLACE "\\n" "\n" RESULT "${STR}")
  string(REPLACE "\\r" "\r" RESULT "${RESULT}")
  string(REPLACE "\\t" "\t" RESULT "${RESULT}")
  string(REPLACE "\\x1b" "${ANSI_ESCAPE}" RESULT "${RESULT}")
  string(REPLACE "\\033" "${ANSI_ESCAPE}" RESULT "${RESULT}")
  string(REPLACE "\\e" "${ANSI_ESCAPE}"  RESULT "${RESULT}")
  string(REPLACE "\\v" ""  RESULT "${RESULT}")
  string(REPLACE "\\f" ""   RESULT "${RESULT}")
  string(REPLACE "\\\\" "\\" RESULT "${RESULT}")
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif(OUTPUT_VAR)
endfunction()

# ITEMS contains multiple items
#
# assign_items <ITEMS> [VAR0, ...]
#
# Assign the elements of the list ITEMS, in order, to the variables VAR0, ...
# (a macro).
#
macro(assign_items ITEMS)
  set(__L "${ITEMS}")
  set(__I 0)
  foreach(__A ${ARGN})
    list(GET __L "${__I}" "${__A}")
    math(EXPR __I "${__I} + 1")
 endforeach()
endmacro()

# LIST is the name of a list
#
# assign_list <LIST> [VAR0...]
#
# Like assign_items, but LIST is the name of a list variable.
#
macro(assign_list LIST)
  assign_items("${${LIST}}" ${ARGN})
endmacro()

#
# eat_line <BUFFER-VAR> <RESULT-VAR>
#
# Move the first line of BUFFER-VAR into RESULT-VAR and leave the rest in
# BUFFER-VAR.
#
function(eat_line BUFFER_VAR RESULT_VAR)
  set(BUF "${${BUFFER_VAR}}")
  string(FIND "${BUF}" "\n" NL_POS)
  string(LENGTH "${BUF}" LEN)
  if(${NL_POS} EQUAL -1)
    set(NL_POS "${LEN}")
    set(NEXT_POS "${NL_POS}")
  else()
    math(EXPR NEXT_POS "${NL_POS} + 1")
  endif()

  string(SUBSTRING "${BUF}" "0" "${NL_POS}" RESULT)
  string(SUBSTRING "${BUF}"  "${NEXT_POS}"  -1 REST)
  set("${RESULT_VAR}" "${RESULT}" PARENT_SCOPE)
  set("${BUFFER_VAR}" "${REST}" PARENT_SCOPE)
endfunction()

#
# add_prefix <OUTPUT-VAR> <PREFIX> [ARGS...]
#
# Store ARGS, each with PREFIX prepended, in OUTPUT-VAR.
#
macro(add_prefix OUTPUT_VAR PREFIX)
  unset("${OUTPUT_VAR}" PARENT_SCOPE)
  foreach(ARG ${ARGN})
    list(APPEND "${OUTPUT_VAR}" "${PREFIX}${ARG}")
  endforeach()
endmacro()

#
# add_suffix <OUTPUT-VAR> <SUFFIX> [ARGS...]
#
# Store ARGS, each with SUFFIX appended, in OUTPUT-VAR.
#
macro(add_suffix OUTPUT_VAR SUFFIX)
  unset("${OUTPUT_VAR}" PARENT_SCOPE)
  foreach(ARG ${ARGN})
    list(APPEND "${OUTPUT_VAR}" "${ARG}${SUFFIX}")
  endforeach()
endmacro()

#
# assign_named_prefix <PREFIX> [ARGS...]
#
# Treat ARGS as alternating NAME/VALUE lines and set the cache variable
# PREFIX<NAME> to VALUE for each pair.
#
macro(assign_named_prefix PREFIX)
  set(ARGUMENTS "${ARGN}")
  while(NOT "${ARGUMENTS}" STREQUAL "")
    eat_line(ARGUMENTS NAME)
    eat_line(ARGUMENTS VALUE)
   set("${PREFIX}${NAME}" "${VALUE}" CACHE STRING "Color value")
  endwhile()
endmacro()

#
# assign_named_items [ARGS...]
#
# Treat ARGS as alternating NAME VALUE pairs and set each variable NAME to
# VALUE in the caller's scope.
#
macro(assign_named_items)
 unset(__N)
  foreach(__A ${ARGN})
     if(NOT DEFINED __N)
      set(__N "${__A}")
     else()
      set(__V "${__A}")
      endif()
     if(DEFINED __V)
         set("${__N}" "${__V}")
       unset(__N)
       unset(__V)
     endif()
  endforeach()
endmacro()

#
# assign_named_var <VAR_NAME>
#
# Apply assign_named_items to the pairs held in the list variable VAR_NAME.
#
macro(assign_named_var VAR_NAME)
  assign_named_items(${${VAR_NAME}})
endmacro()

#
# isin_var <OUTPUT-VAR> <ITEM> <VAR-NAME>
#
# Store TRUE in OUTPUT-VAR if ITEM is in the list variable VAR-NAME,
# otherwise FALSE.
#
function(isin_var OUTPUT_VAR ITEM VAR_NAME)
  if(ITEM IN_LIST "${VAR_NAME}")
    set(RESULT TRUE)
  else()
    set(RESULT FALSE)
  endif()

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# isin_list <OUTPUT-VAR> <ITEM> [LIST...]
#
# Store TRUE in OUTPUT-VAR if ITEM is one of the LIST elements, otherwise
# FALSE.
#
function(isin_list OUTPUT_VAR ITEM)
  set(LIST "${ARGN}")
  isin_var("${OUTPUT_VAR}" "${ITEM}" LIST)
endfunction()

#
# absolute_paths <OUTPUT-VAR> [PATHS...]
#
# Store PATHS, made absolute relative to the current directory, in OUTPUT-VAR.
#
function(absolute_paths OUTPUT_VAR)
  set(RESULT "")
  foreach(ARG ${ARGN})
      cmake_path(ABSOLUTE_PATH ARG OUTPUT_VARIABLE VALUE)
      list(APPEND RESULT "${VALUE}")
  endforeach()

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# absolute_paths_in <OUTPUT-VAR> <BASE-DIRECTORY> [PATHS...]
#
# Store PATHS, made absolute relative to BASE-DIRECTORY, in OUTPUT-VAR.
#
function(absolute_paths_in OUTPUT_VAR BASE_DIRECTORY)
  set(RESULT "")
  foreach(ARG ${ARGN})
      cmake_path(ABSOLUTE_PATH ARG BASE_DIRECTORY "${BASE_DIRECTORY}" OUTPUT_VARIABLE VALUE)
      list(APPEND RESULT "${VALUE}")
  endforeach()

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# relative_paths <OUTPUT-VAR> <BASE-DIRECTORY> [PATHS...]
#
# Store PATHS, made relative to BASE-DIRECTORY, in OUTPUT-VAR.
#
function(relative_paths OUTPUT_VAR BASE_DIRECTORY)
  set(RESULT "")
  foreach(ARG ${ARGN})
      cmake_path(RELATIVE_PATH ARG BASE_DIRECTORY "${BASE_DIRECTORY}" OUTPUT_VARIABLE VALUE)
      list(APPEND RESULT "${VALUE}")
  endforeach()

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# get_cwd <OUTPUT-VAR>
#
# Store the absolute path of the current directory in OUTPUT-VAR.
#
function(get_cwd OUTPUT_VAR)
  set(ARG ".")
  cmake_path(ABSOLUTE_PATH ARG OUTPUT_VARIABLE RESULT)
  string(REGEX REPLACE "[/\\\\]\\.$" "" RESULT "${RESULT}")
  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# is_relative <PATH> <OUTPUT-VAR>
#
# Check whether PATH is a relative path.
#
function(is_relative PATH OUTPUT_VAR)
  if(OUTPUT_VAR)
    cmake_path(IS_RELATIVE PATH "${OUTPUT_VAR}")
  endif()
endfunction()

#
# is_absolute <PATH> <OUTPUT-VAR>
#
# Check whether PATH is an absolute path.
#
function(is_absolute PATH OUTPUT_VAR)
  if(OUTPUT_VAR)
    cmake_path(IS_ABSOLUTE PATH "${OUTPUT_VAR}")
  endif()
endfunction()

#
# concat <OUTPUT-VAR> <SEPARATOR> [ARGUMENTS...]
#
# Join ARGUMENTS with SEPARATOR between them and store the result in
# OUTPUT-VAR.
#
function(concat OUTPUT_VAR SEPARATOR)
  set(RESULT "")
  foreach(ARG ${ARGN})
    if(NOT "${RESULT}" STREQUAL "")
      set(RESULT "${RESULT}${SEPARATOR}${ARG}")
    else()
      set(RESULT "${ARG}")
    endif()
  endforeach()

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# basename <OUTPUT-VAR> <STR> [EXT_NAME]
#
# Store STR without its leading directories, and without the extension
# EXT_NAME if given, in OUTPUT-VAR.
#
function(basename OUTPUT_VAR STR)
  string(REGEX REPLACE ".*/" "" RESULT "${STR}")
  if(ARGN)
    string(REGEX REPLACE "\\${ARGN}\$" "" RESULT "${RESULT}")
  endif()

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# var2define <NAME> [DEFINED_VALUE] [VAR_NAME]
#
# Add the compile definition -DNAME: =1 or =0 by the truth of variable NAME,
# or =DEFINED_VALUE if VAR_NAME is true.
#
function(var2define NAME)
  if(${ARGC} GREATER_EQUAL 3)
    set(VAR_NAME "${ARGV1}")
  else()
    set(VAR_NAME "${NAME}")
  endif()

  set(VALUE "${${VAR_NAME}}")
  if(${ARGC} LESS_EQUAL 1 AND ${ARGC} GREATER_EQUAL 0)
    if(VALUE)
      add_definitions(-D${NAME}=1)
    else()
      add_definitions(-D${NAME}=0)
    endif()
  else()
    if(VALUE)
      add_definitions(-D${NAME}=${ARGV1})
    endif()
  endif()
endfunction()

# Get number of columns the terminal supports
#
# get_columns <OUTPUT-VAR> [DEFAULT]
#
# Store the width of the terminal in OUTPUT-VAR, from $COLUMNS or tput, or
# DEFAULT (80) if unknown.
#
function(get_columns RESULT_VAR)
  if(${ARGC} GREATER_EQUAL 2)
    set(DEFAULT_VALUE ${ARGV1})
  else()
    set(DEFAULT_VALUE 80)
  endif()

  set(RESULT "$ENV{COLUMNS}")
  if(RESULT LESS 1)
    execute_process(COMMAND tput cols OUTPUT_VARIABLE TPUT_COLS ERROR_QUIET ERROR_VARIABLE TPUT_ERROR)
   
    if(NOT TPUT_ERROR AND TPUT_COLS)
      set(RESULT ${TPUT_COLS})
    else()
      set(SOURCE_NAME ttysize.c)
      set(SOURCE_CODE "#include <unistd.h>\n#include <fcntl.h>\n#include <termios.h>\n#include <sys/ioctl.h>\n#include <stdio.h>\n\nint\nmain() {\n\tstruct winsize sz;\n\tint fd = isatty(0) ? dup(0) : open(\"/dev/tty\", O_RDWR);\n\n\tif(!isatty(fd)) {\n\t\tfputs(\"not a tty\\n\", stderr);\n\t\tfflush(stderr);\n\t\treturn 1;\n\t}\n\n\tif(ioctl(fd, TIOCGWINSZ, &sz) == -1) {\n\t\tperror(\"ioctl\");\n\t\treturn 1;\n\t}\n\n\tclose(fd);\n\n\tprintf(\"%u\\n\", sz.ws_col);\n\treturn 0;\n}\n")
      message(CHECK_START "Trying to compile ${SOURCE_NAME}")
      try_run(RUN_RESULT COMPILE_RESULT SOURCE_FROM_CONTENT "${SOURCE_NAME}" "${SOURCE_CODE}" RUN_OUTPUT_STDOUT_VARIABLE RUN_OUTPUT COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT NO_CACHE)
   
      if(NOT COMPILE_RESULT)
        message(CHECK_FAIL "failed to compile:\n${COMPILE_OUTPUT}")
      else()
        if(RUN_RESULT STREQUAL 0)
          message(CHECK_PASS "ok")
          set(RESULT "${RUN_OUTPUT}")
        else()
          message(CHECK_FAIL "failed to run:\n${RUN_OUTPUT}")
        endif()
      endif()
    endif()
  endif()

  if(NOT RESULT GREATER_EQUAL 1)
    set(RESULT "${DEFAULT_VALUE}")
  endif()

  if(RESULT_VAR)
    set("${RESULT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# named_dump <MSG> [VAR-NAMES...]
#
# Print MSG followed by the name and value of each of VAR-NAMES, laid out as
# set by DUMP_FORMAT.
#
function(named_dump MSG)
  assign_named_var(DUMP_FORMAT)
  set(INDEX 0)
  math(EXPR LAST "${ARGC} - 2")
  set(RESULT "${MSG}${START}")
  
  foreach(VAR_NAME ${ARGN})
    set(VALUE "${${VAR_NAME}}")
    set(LINE "${INDENT}${VAR_NAME}${PROP}${QUOTE}${VALUE}${QUOTE}")
    if(INDEX LESS LAST)
      set(LINE "${LINE}${COMMA}")
    endif()
    set(RESULT "${RESULT}${NEWLINE}${LINE}")
    math(EXPR INDEX "${INDEX} + 1")
  endforeach()

  if(NOT END STREQUAL "")
    set(RESULT "${RESULT}${NEWLINE}${END}")
  endif()

  message("${RESULT}")
endfunction()

set(DUMP_FORMAT INDENT "  " START " {" END "}" PROP ": " QUOTE "'" COMMA "," NEWLINE "\\n")

#
# dump [VAR-NAMES...]
#
# Print the name and value of each of VAR-NAMES.
#
function(dump)
  named_dump("Variable dump" ${ARGN})
endfunction()

#
# dump_list <VAR-NAME>
#
# Print every element of the list variable VAR-NAME on its own numbered line.
#
function(dump_list)
  assign_named_var(DUMP_FORMAT)
  foreach(VAR_NAME ${ARGN})
    message("List dump of ${VAR_NAME}:")
    list(LENGTH "${VAR_NAME}" NUM_ITEMS)
    set(INDEX 0)
    while(${INDEX} LESS ${NUM_ITEMS})
      list(GET "${VAR_NAME}" "${INDEX}" ITEM)
      message("${INDENT}${INDEX}: ${ITEM}")
      math(EXPR INDEX "${INDEX} + 1")
    endwhile()
  endforeach()
endfunction()

#
# make_list <OUTPUT-VAR> <MAX_LINE_LEN>
#
# Store the words in the arguments in OUTPUT-VAR as lines no longer than
# MAX_LINE_LEN, laid out as set by LIST_FORMAT.
#
function(make_list OUTPUT_VAR MAX_LINE_LEN)
  assign_named_var(LIST_FORMAT)
  set(RESULT "")
  set(LINE "")
  string(REPLACE " " ";" ARGS "${ARGN}")
  foreach(ITEM ${ARGS})
    string(LENGTH "${LINE}${SEP}${ITEM}" LEN)
    math(EXPR EFFECTIVE_LEN "${LEN} + 4")
    if(EFFECTIVE_LEN GREATER MAX_LINE_LEN)
      set(RESULT "${RESULT}\n${INDENT}${LINE}")
      set(LINE " ${ITEM}")
      string(LENGTH "${LINE}" LEN)
    else()
      set(LINE "${LINE}${SEP}${ITEM}")
    endif()
  endforeach()

  if(LINE)
    set(RESULT "${RESULT}\n${INDENT}${LINE}")
  endif()

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

set(LIST_FORMAT INDENT "    " SEP  " ")

#
# make_columns <OUTPUT-VAR> <MAX_LINE_LEN> [ITEMS...]
#
# Store the ITEMS in OUTPUT-VAR as lines of aligned columns, filled column by
# column like ls(1), using as many columns as fit in MAX_LINE_LEN; the layout
# is set by COLUMN_FORMAT.
#
function(make_columns OUTPUT_VAR MAX_LINE_LEN)
  assign_named_var(COLUMN_FORMAT)
  set(ITEMS ${ARGN})
  list(LENGTH ITEMS COUNT)
  set(RESULT "")

  if(COUNT GREATER 0)
    string(LENGTH "${INDENT}" INDENT_LEN)
    string(LENGTH "${GAP}" GAP_LEN)

    set(LENGTHS "")
    foreach(ITEM ${ITEMS})
      string(LENGTH "${ITEM}" LEN)
      list(APPEND LENGTHS ${LEN})
    endforeach()

    # try the most columns first, until the widest layout that fits is found
    set(NCOLS ${COUNT})
    while(TRUE)
      math(EXPR ROWS "(${COUNT} + ${NCOLS} - 1) / ${NCOLS}")
      math(EXPR USED "(${COUNT} + ${ROWS} - 1) / ${ROWS}")

      set(WIDTHS "")
      set(TOTAL ${INDENT_LEN})
      foreach(COL RANGE 0 ${USED})
        if(COL EQUAL USED)
          break()
        endif()
        math(EXPR FIRST "${COL} * ${ROWS}")
        math(EXPR LAST "${FIRST} + ${ROWS} - 1")
        if(LAST GREATER_EQUAL COUNT)
          math(EXPR LAST "${COUNT} - 1")
        endif()
        set(WIDTH 0)
        foreach(I RANGE ${FIRST} ${LAST})
          list(GET LENGTHS ${I} LEN)
          if(LEN GREATER WIDTH)
            set(WIDTH ${LEN})
          endif()
        endforeach()
        list(APPEND WIDTHS ${WIDTH})
        math(EXPR TOTAL "${TOTAL} + ${WIDTH}")
        if(COL GREATER 0)
          math(EXPR TOTAL "${TOTAL} + ${GAP_LEN}")
        endif()
      endforeach()

      if(TOTAL LESS_EQUAL MAX_LINE_LEN OR NCOLS EQUAL 1)
        break()
      endif()
      math(EXPR NCOLS "${NCOLS} - 1")
    endwhile()

    foreach(ROW RANGE 0 ${ROWS})
      if(ROW EQUAL ROWS)
        break()
      endif()
      set(LINE "")
      foreach(COL RANGE 0 ${USED})
        if(COL EQUAL USED)
          break()
        endif()
        math(EXPR I "${COL} * ${ROWS} + ${ROW}")
        if(I LESS COUNT)
          list(GET ITEMS ${I} ITEM)
          math(EXPR NEXT "(${COL} + 1) * ${ROWS} + ${ROW}")
          if(NEXT LESS COUNT)
            list(GET WIDTHS ${COL} WIDTH)
            list(GET LENGTHS ${I} LEN)
            math(EXPR PAD "${WIDTH} - ${LEN}")
            string(REPEAT " " ${PAD} SPACES)
            set(LINE "${LINE}${ITEM}${SPACES}${GAP}")
          else()
            set(LINE "${LINE}${ITEM}")
          endif()
        endif()
      endforeach()
      set(RESULT "${RESULT}\n${INDENT}${LINE}")
    endforeach()
  endif()

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

set(COLUMN_FORMAT INDENT "  " GAP "  ")
  
#
# print_list <DESC> <VAR-NAME> [COLUMNS]
#
# Print DESC, the number of elements and the list variable VAR-NAME, wrapped
# to COLUMNS (or the terminal width).
#
function(print_list DESC VAR_NAME)
  list(LENGTH "${VAR_NAME}" COUNT)
  if(COUNT LESS_EQUAL 0)
    message(STATUS "${DESC} (0): empty")
  else()
    if(ARGC GREATER_EQUAL 3)
      set(COLUMNS ${ARGV2})
    elseif(NOT DEFINED MAX_COLUMNS)
      get_columns(COLUMNS)
      set(MAX_COLUMNS "${COLUMNS}" PARENT_SCOPE)
    endif()
    make_list(L ${COLUMNS} ${${VAR_NAME}})
    message("${DESC} (${COUNT}): ${L}\n")
  endif()
endfunction()

#
# print_columns <DESC> <VAR-NAME> [COLUMNS]
#
# Print DESC, the number of elements and the list variable VAR-NAME, wrapped
# to COLUMNS (or the terminal width).
#
function(print_columns DESC VAR_NAME)
  list(LENGTH "${VAR_NAME}" COUNT)
  if(COUNT LESS_EQUAL 0)
    message(STATUS "${DESC} (0): empty")
  else()
    if(ARGC GREATER_EQUAL 3)
      set(COLUMNS ${ARGV2})
    elseif(NOT DEFINED MAX_COLUMNS)
      get_columns(COLUMNS)
      set(MAX_COLUMNS "${COLUMNS}" PARENT_SCOPE)
    endif()
    make_columns(L ${COLUMNS} ${${VAR_NAME}})
    message("${DESC} (${COUNT})\n${L}\n")
  endif()
endfunction()

#
# make_filter_re <OUTPUT-VAR> [LIST...]
#
# Store a regular expression in OUTPUT-VAR that matches any LIST element,
# with '-' and '_' interchangeable.
#
function(make_filter_re OUTPUT_VAR)
  set(LIST "")
  list(APPEND LIST ${ARGN})
  list(JOIN LIST "|" RE)
  string(REGEX REPLACE "[-_]" "[-_]" RE "(${RE})")
  set("${OUTPUT_VAR}" "${RE}" PARENT_SCOPE)
endfunction()

#
# message_unescaped [STRINGS...]
#
# Print STRINGS joined together, after turning escape sequences such as \n
# and \e into characters.
#
function(message_unescaped)
  set(S "")

  foreach(ARG ${ARGN})
    set(S "${S}${ARG}")
  endforeach()

  unescape_string(S "${S}")
  message("${S}")
endfunction()

#
# message_func <FUNCTION-NAME> [STRINGS...]
#
# Print the colored function name FUNCTION-NAME followed by STRINGS (for
# tracing calls).
#
function(message_func FUNC)
  set(S "${COLOR_LIGHTRED}${FUNC}${COLOR_NONE}")
  
  foreach(ARG ${ARGN})
    set(S "${S} ${ARG}")
  endforeach()

  unescape_string(S "${S}")
  message("${S}")
endfunction()

#
# init_colors
#
# Define the ANSI escape sequence variables COLOR_NONE, COLOR_RED, ... used
# for colored messages (a macro).
#
macro(init_colors)
  set(ANSI_ESCAPE "")
  set(SEMI "╎")
  set(COLORS
    NONE "${ANSI_ESCAPE}[0m"
    BLACK "${ANSI_ESCAPE}[0${SEMI}30m"
    RED "${ANSI_ESCAPE}[0${SEMI}31m"
    GREEN "${ANSI_ESCAPE}[0${SEMI}32m"
    BROWN "${ANSI_ESCAPE}[0${SEMI}33m"
    BLUE "${ANSI_ESCAPE}[0${SEMI}34m"
    MAGENTA "${ANSI_ESCAPE}[0${SEMI}35m"
    CYAN "${ANSI_ESCAPE}[0${SEMI}36m"
    LIGHTGRAY "${ANSI_ESCAPE}[0${SEMI}37m"
    DARKGRAY "${ANSI_ESCAPE}[1${SEMI}30m"
    LIGHTRED "${ANSI_ESCAPE}[1${SEMI}31m"
    LIGHTGREEN "${ANSI_ESCAPE}[1${SEMI}32m"
    YELLOW "${ANSI_ESCAPE}[1${SEMI}33m"
    LIGHTBLUE "${ANSI_ESCAPE}[1${SEMI}34m"
    LIGHTMAGENTA "${ANSI_ESCAPE}[1${SEMI}35m"
    LIGHTCYAN "${ANSI_ESCAPE}[1${SEMI}36m"
    WHITE "${ANSI_ESCAPE}[1${SEMI}37m"
  )
  string(REPLACE ";" "\n" CMAP "${COLORS}")
  string(REPLACE "${SEMI}" ";" CMAP "${CMAP}")
  assign_named_prefix(COLOR_ ${CMAP})
endmacro()

#
# getenv <OUTPUT-VAR> <VAR-NAME>
#
# Store the value of the environment variable VAR-NAME in OUTPUT-VAR, or
# unset OUTPUT-VAR if it is not set.
#
function(getenv OUTPUT_VAR VAR_NAME)
  if(OUTPUT_VAR)
    if(DEFINED ENV{${VAR_NAME}})
      set("${OUTPUT_VAR}" "$ENV{${VAR_NAME}}" PARENT_SCOPE)
    else()
      unset("${OUTPUT_VAR}" PARENT_SCOPE)
    endif()
  endif()
endfunction()

#
# getenv_default <OUTPUT-VAR> <VAR-NAME> [DEFAULT-VALUE]
#
# Store the value of the environment variable VAR-NAME in OUTPUT-VAR, or
# DEFAULT-VALUE if it is not set.
#
function(getenv_default OUTPUT_VAR VAR_NAME)
  if(DEFINED ENV{${VAR_NAME}})
    set(RESULT "$ENV{${VAR_NAME}}")
  else()
    set(RESULT "${ARGN}")
  endif()

  if(OUTPUT_VAR)
    set("${OUTPUT_VAR}" "${RESULT}" PARENT_SCOPE)
  endif()
endfunction()

#
# Functions carried over from the previous functions.cmake of this project.
#
include(CheckFunctionExists)

##
## canonicalize <OUTPUT VARIABLE> <STR>
##
function(CANONICALIZE OUTPUT_VAR STR)
  string(REGEX REPLACE "^-W" "WARN_" TMP_STR "${STR}")

  string(REGEX REPLACE "-" "_" TMP_STR "${TMP_STR}")
  string(TOUPPER "${TMP_STR}" TMP_STR)

  set("${OUTPUT_VAR}" "${TMP_STR}" PARENT_SCOPE)
endfunction(CANONICALIZE OUTPUT_VAR STR)

##
## dirname <OUTPUT VARIABLE> <STR>
##
function(DIRNAME OUTPUT_VAR STR)
  string(REGEX REPLACE "/[^/]+/*$" "" TMP_STR "${STR}")
  if(ARGN)
    string(REGEX REPLACE "\\${ARGN}\$" "" TMP_STR "${TMP_STR}")
  endif(ARGN)

  set("${OUTPUT_VAR}" "${TMP_STR}" PARENT_SCOPE)
endfunction(DIRNAME OUTPUT_VAR FILE)

##
## addprefix <OUTPUT VARIABLE> <PREFIX>
##
function(ADDPREFIX OUTPUT_VAR PREFIX)
  set(OUTPUT "")
  foreach(ARG ${ARGN})
    list(APPEND OUTPUT "${PREFIX}${ARG}")
  endforeach(ARG ${ARGN})
  set("${OUTPUT_VAR}" "${OUTPUT}" PARENT_SCOPE)
endfunction(ADDPREFIX OUTPUT_VAR PREFIX)

##
## addsuffix <OUTPUT VARIABLE> <PREFIX>
##
function(ADDSUFFIX OUTPUT_VAR SUFFIX)
  set(OUTPUT "")
  foreach(ARG ${ARGN})
    list(APPEND OUTPUT "${ARG}${SUFFIX}")
  endforeach(ARG ${ARGN})
  set("${OUTPUT_VAR}" "${OUTPUT}" PARENT_SCOPE)
endfunction(ADDSUFFIX OUTPUT_VAR SUFFIX)

##
## relative_path <OUTPUT VARIABLE> <RELATIVE_TO>
##
function(RELATIVE_PATH OUT_VAR RELATIVE_TO)
  set(LIST "")

  foreach(ARG ${ARGN})
    file(RELATIVE_PATH ARG "${RELATIVE_TO}" "${ARG}")
    list(APPEND LIST "${ARG}")
  endforeach(ARG ${ARGN})

  set("${OUT_VAR}" "${LIST}" PARENT_SCOPE)
endfunction(RELATIVE_PATH RELATIVE_TO OUT_VAR)

##
## check_function_def <FUNCTION NAME> [RESULT VARIABLE] [PREPROC_DEF]
##
macro(CHECK_FUNCTION_DEF FUNC)
  if(${ARGC} GREATER 1)
    set(RESULT_VAR "${ARGV1}")
  else(${ARGC} GREATER 1)
    string(TOUPPER "HAVE_${FUNC}" RESULT_VAR)
  endif(${ARGC} GREATER 1)

  if(${ARGC} GREATER 2)
    set(PREPROC_DEF "${ARGV2}")
  else(${ARGC} GREATER 2)
    string(TOUPPER "HAVE_${FUNC}" PREPROC_DEF)
  endif(${ARGC} GREATER 2)

  if(NOT DEFINED ${RESULT_VAR})
    check_function_exists("${FUNC}" "_${RESULT_VAR}")

    if(${_${RESULT_VAR}})
      set("${RESULT_VAR}" TRUE
          CACHE INTERNAL "Define this if you have the '${FUNC}' function")
    else(${_${RESULT_VAR}})
      set("${RESULT_VAR}" FALSE
          CACHE INTERNAL "Define this if you have the '${FUNC}' function")
    endif(${_${RESULT_VAR}})
  endif(NOT DEFINED ${RESULT_VAR})

  set(DEFINE FALSE)

  if(${${RESULT_VAR}})
    if(NOT "${PREPROC_DEF}" STREQUAL "")
      set("${PREPROC_DEF}" "1")
      var2define("${PREPROC_DEF}" 1)
    endif(NOT "${PREPROC_DEF}" STREQUAL "")
  endif(${${RESULT_VAR}})

  #message("${RESULT_VAR}: ${${RESULT_VAR}}")

  list(APPEND CHECKED_FUNCTIONS "${FUNC}")
endmacro(CHECK_FUNCTION_DEF FUNC)

##
## check_functions <FUNCTION NAMES...>
##
macro(CHECK_FUNCTIONS)
  foreach(FUNC ${ARGN})
    string(TOUPPER "HAVE_${FUNC}" RESULT_VAR)
    check_function_def("${FUNC}" "${RESULT_VAR}")
  endforeach(FUNC ${ARGN})
endmacro(CHECK_FUNCTIONS)

##
## check_functions_def <FUNCTION NAMES...>
##
macro(CHECK_FUNCTIONS_DEF)
  foreach(FUNC ${ARGN})
    check_function_def("${FUNC}")
  endforeach(FUNC ${ARGN})
endmacro(CHECK_FUNCTIONS_DEF)

##
## clean_name <STRING> <OUTPUT VAR>
##
function(CLEAN_NAME STR OUTPUT_VAR)
  string(TOUPPER "${STR}" STR)
  string(REGEX REPLACE "[^A-Za-z0-9_]" "_" STR "${STR}")
  set("${OUTPUT_VAR}" "${STR}" PARENT_SCOPE)
endfunction(CLEAN_NAME STR OUTPUT_VAR)

##
## check_include_def <INCLUDE> [RESULT VARIABLE] [PREPROC_DEF]
##
macro(CHECK_INCLUDE_DEF INC)
  if(ARGC GREATER_EQUAL 2)
    set(RESULT_VAR "${ARGV1}")
    set(PREPROC_DEF "${ARGV2}")
  else(ARGC GREATER_EQUAL 2)
    clean_name("${INC}" INC_D)
    string(TOUPPER "HAVE_${INC_D}" RESULT_VAR)
    string(TOUPPER "HAVE_${INC_D}" PREPROC_DEF)
  endif(ARGC GREATER_EQUAL 2)

  check_include_file("${INC}" "${RESULT_VAR}")

  if(${${RESULT_VAR}})
    set("${RESULT_VAR}" TRUE
        CACHE INTERNAL "Define this if you have the '${INC}' header file")

    if(NOT "${PREPROC_DEF}" STREQUAL "")
      var2define("${PREPROC_DEF}" 1)
    endif(NOT "${PREPROC_DEF}" STREQUAL "")
  endif(${${RESULT_VAR}})

  list(APPEND CHECKED_INCLUDES "${INC}")
endmacro(CHECK_INCLUDE_DEF INC)

##
## check_includes <INCLUDE FILES...>
##
macro(CHECK_INCLUDES)
  foreach(INC ${ARGN})
    clean_name("HAVE_${INC}" RESULT_VAR)
    check_include_def("${INC}" "${RESULT_VAR}")
  endforeach(INC ${ARGN})
endmacro(CHECK_INCLUDES)

##
## check_includes_def <INCLUDE FILES...>
##
macro(CHECK_INCLUDES_DEF)
  foreach(INC ${ARGN})
    check_include_def("${INC}")
  endforeach(INC ${ARGN})
endmacro(CHECK_INCLUDES_DEF)

##
## check_function_and_include <FUNCTION> <INCLUDE>
##
macro(CHECK_FUNCTION_AND_INCLUDE FUNC INC)
  clean_name("HAVE_${INC}" INC_RESULT)
  clean_name("HAVE_${FUNC}" FUNC_RESULT)

  check_include_def("${INC}" "${INC_RESULT}" "${INC_RESULT}")

  if(${${INC_RESULT}})
    check_function_def("${FUNC}" "${FUNC_RESULT}" "${FUNC_RESULT}")
  endif(${${INC_RESULT}})
endmacro(CHECK_FUNCTION_AND_INCLUDE FUNC INC)

##
## check_include_cxx_def <INCLUDE> [RESULT VARIABLE] [PREPROC_DEF]
##
macro(CHECK_INCLUDE_CXX_DEF INC)
  if(ARGC GREATER_EQUAL 2)
    set(RESULT_VAR "${ARGV1}")
    set(PREPROC_DEF "${ARGV2}")
  else(ARGC GREATER_EQUAL 2)
    clean_name("${INC}" INC_D)
    string(TOUPPER "HAVE_${INC_D}" RESULT_VAR)
    string(TOUPPER "HAVE_${INC_D}" PREPROC_DEF)
  endif(ARGC GREATER_EQUAL 2)

  check_include_file_cxx("${INC}" "${RESULT_VAR}")

  if(${${RESULT_VAR}})
    set("${RESULT_VAR}" TRUE
        CACHE INTERNAL "Define this if you have the '${INC}' header file")

    if(NOT "${PREPROC_DEF}" STREQUAL "")
      var2define("${PREPROC_DEF}" 1)
    endif(NOT "${PREPROC_DEF}" STREQUAL "")
  endif(${${RESULT_VAR}})
endmacro(CHECK_INCLUDE_CXX_DEF INC)

##
## append_parent <VARIABLE NAME>
##
macro(APPEND_PARENT VAR)
  set(LIST "${${VAR}}")
  list(APPEND LIST ${ARGN})
  set("${VAR}" "${LIST}" PARENT_SCOPE)
endmacro(APPEND_PARENT VAR)

##
## contains <LIST NAME> <VALUE> <OUTPUT VARIABE>
##
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

##
## add_unique <LIST NAME> <VALUES...>
##
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

##
## symlink <TARGET> <SYMLINK PATH>
##
macro(SYMLINK TARGET LINK_NAME)
  install(
    CODE "message(\"Create symlink '$ENV{DESTDIR}${LINK_NAME}' to '${TARGET}'\")\nexecute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${TARGET} $ENV{DESTDIR}${LINK_NAME})"
  )
endmacro(SYMLINK TARGET LINK_NAME)

##
## rpath_append <VARIABLE NAME>
##
macro(RPATH_APPEND VAR)
  foreach(VALUE ${ARGN})
    if("${${VAR}}" STREQUAL "")
      set(${VAR} "${VALUE}")
    else("${${VAR}}" STREQUAL "")
      set(${VAR} "${CMAKE_INSTALL_RPATH}:${VALUE}")
    endif("${${VAR}}" STREQUAL "")
  endforeach(VALUE ${ARGN})
endmacro(RPATH_APPEND VAR)

##
## try_code <FILENAME> <CODE> <RESULT VARIABLE> <OUTPUT VARIABLE> <LIBS> <LINKER FLAGS>
##
function(TRY_CODE FILE CODE RESULT_VAR OUTPUT_VAR LIBS LDFLAGS)
  if(NOT DEFINED "${RESULT_VAR}" OR NOT DEFINED "${OUTPUT_VAR}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${FILE}" "${CODE}")

    try_compile(
      RESULT "${CMAKE_CURRENT_BINARY_DIR}"
      "${CMAKE_CURRENT_BINARY_DIR}/${FILE}"
      CMAKE_FLAGS "${CMAKE_REQUIRED_FLAGS}"
      COMPILE_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}"
      LINK_OPTIONS "${LDFLAGS}"
      LINK_LIBRARIES "${LIBS}"
      OUTPUT_VARIABLE OUTPUT)

    set(${RESULT_VAR} "${RESULT}" PARENT_SCOPE)
    set(${OUTPUT_VAR} "${OUTPUT}" PARENT_SCOPE)
  endif(NOT DEFINED "${RESULT_VAR}" OR NOT DEFINED "${OUTPUT_VAR}")
endfunction()

##
## check_external <NAME> <LIBS> <LINKER FLAGS> <OUTPUT VARIABLE>
##
function(CHECK_EXTERNAL NAME LIBS LDFLAGS OUTPUT_VAR)
  try_code(
    "test-${NAME}.c"
    "\n  extern int ${NAME}(void);\n  int main() {\n    ${NAME}();\n    return 0;\n  }\n  "
    "${OUTPUT_VAR}"
    OUT
    "${LIBS}"
    "${LDFLAGS}")
  #dump(OUTPUT_VAR OUT)
endfunction(CHECK_EXTERNAL NAME LIBS LDFLAGS OUTPUT_VAR)

##
## run_code <FILENAME> <CODE> <RESULT VARIABLE> <OUTPUT VARIABLE> <LIBS> <LINKER FLAGS>
##
function(RUN_CODE FILE CODE RESULT_VAR OUTPUT_VAR LIBS LDFLAGS)
  string(RANDOM LENGTH 8 RND)
  set(FN "${CMAKE_CURRENT_BINARY_DIR}/${RND}-${FILE}")
  file(WRITE "${FN}" "${CODE}")
  string(REGEX REPLACE "\.[^./]+$" ".log" LOG "${FN}")

  try_run(
    RUN_RESULT COMPILE_RESULT SOURCES "${FN}"
    COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT
    RUN_OUTPUT_VARIABLE RUN_OUTPUT
    CMAKE_FLAGS "${CMAKE_REQUIRED_FLAGS}"
    COMPILE_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS}" LINK_OPTIONS
                        "${LDFLAGS}" LINK_LIBRARIES "${LIBS}")

  file(WRITE "${LOG}"
       "Compile output:\n${COMPILE_OUTPUT}\n\nRun output:\n${RUN_OUTPUT}\n")
  unset(LOG)

  set(${RESULT_VAR} "${COMPILE_RESULT}" PARENT_SCOPE)
  set(${OUTPUT_VAR} "${COMPILE_OUTPUT}" PARENT_SCOPE)

  file(REMOVE "${FN}")

  if(COMPILE_RESULT)
    if(NOT "${RUN_RESULT}" STREQUAL "")
      set(${RESULT_VAR} "${RUN_RESULT}" PARENT_SCOPE)
    endif(NOT "${RUN_RESULT}" STREQUAL "")
    if(NOT "${RUN_OUTPUT}" STREQUAL "")
      set(${OUTPUT_VAR} "${RUN_OUTPUT}" PARENT_SCOPE)
    endif(NOT "${RUN_OUTPUT}" STREQUAL "")
  endif(COMPILE_RESULT)

  file(REMOVE "${FN}")
  unset(FN)
  unset(RND)
endfunction()

##
## libname <OUTPUT VARIABLE> <FILENAME>
##
function(LIBNAME OUT_VAR FILENAME)
  string(REGEX REPLACE ".*/(lib|)" "" LIBNAME "${FILENAME}")
  string(REGEX REPLACE "\.[^/.]+$" "" LIBNAME "${LIBNAME}")

  set(${OUT_VAR} "${LIBNAME}" PARENT_SCOPE)
endfunction(LIBNAME OUT_VAR FILENAME)

##
## check_flag <FLAG> <VARIABLE>
##
function(CHECK_FLAG FLAG VAR)
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
endfunction(CHECK_FLAG FLAG VAR)
