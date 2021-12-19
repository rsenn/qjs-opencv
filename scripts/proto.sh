#!/bin/sh
NL="
"

addline() {
  for ARG; do
    O="${O:+$O$NL}${INDENT-  }$ARG"
  done
}

process_proto() {
  CLASS=
  PREV_CLASS=
  while read -r LINE; do
    O=
    case "$LINE" in
    *".class_name = "*)
      PREV_CLASS="$CLASS"

      CLASS=${LINE##*"="}
      CLASS=${CLASS%"\""*}
      CLASS=${CLASS#*"\""}

      if [ -n "$PREV_CLASS" ]; then
        INDENT= addline "}" ""
      fi

      INDENT= addline "/**" " * @class ${CLASS}" " */"
      INDENT= addline "class $CLASS {"

      addline "/**" " *  @constructor ${CLASS}" " */"
      addline "constructor() {}"

      ;;
    JS_INIT_MODULE*)
      O="}"
      ;;

    *JSCFunctionListEntry*=*)
      LINE=${LINE%%=*}
      LINE=${LINE#*JSCFunctionListEntry}
      LINE=${LINE#" "}
      LINE=${LINE%%"["*}
      SYM=$LINE
      case "$SYM" in
      *static*) STATIC='static' PROTOTYPE='' ;;
      *) STATIC='' PROTOTYPE='prototype' ;;
      esac
      continue
      ;;
    *"_DEF("*)
      [ -z "$CLASS" ] && continue
      MACRO=${LINE%%"("*}

      L=${LINE%"),"*}
      L=${L#"$MACRO("}

      old_IFS=$IFS
      IFS=",$IFS"
      eval "set -- \${L}"
      IFS=$old_IFS

      NAME=${1%'"'}
      NAME=${NAME#'"'}
      shift
      ALIAS=${1%'"'}
      ALIAS=${ALIAS#'"'}
      ARGC=0
      ITERATOR=false
      GETSET=false
      FUNC=false
      addline ""
      case $MACRO in
      *ITERATOR*NEXT*)
        addline "/**"
        addline " * @iterator ${CLASS}${PROTOTYPE:+.$PROTOTYPE}.${NAME}"
        addline " */"
        addline "${STATIC:+$STATIC }get ${NAME}() {}"
        ITERATOR=true
        ;;
      *GETSET*)
        addline "/**"
        addline " * @var ${CLASS}${PROTOTYPE:+.$PROTOTYPE}.${NAME}"
        addline " */"
        addline "${STATIC:+$STATIC }get ${NAME}() {}"
        GETSET=true
        ;;
      *FUNC*)
        addline "/**"
        addline " * @function ${CLASS}${PROTOTYPE:+.$PROTOTYPE}.${NAME}"
        [ -n "$ARGC" -a ${ARGC:-0} -gt 0 ] && addline " * @param {Object} value  "
        addline " * @returns {Object}  Returns null"
        addline " */"
        addline "${STATIC:+$STATIC }${NAME}() {}"
        ARGC=$1
        FUNC=true
        ;;
      *INT32* | *STRING* | *CONST* | *DOUBLE*)
        addline "/**"
        case "$MACRO" in
        *INT* | *DOUBLE* | *CONST*) TYPE=number ;;
        *STR*) TYPE=string ;;
        esac

        addline " * @var {$TYPE} ${CLASS}${PROTOTYPE:+.$PROTOTYPE}.${NAME}"
        addline " */"
        VALUE="$1"
        case $TYPE in
        number)
          case "$VALUE" in
          [0-9]*) ;;
          *) VALUE=0 ;;
          esac
          ;;
        string)
          case "$VALUE" in
          '"'*'"' | "'"*"'") ;;
          *) VALUE="''" ;;
          esac
          ;;
        esac

        addline "${STATIC:+$STATIC }${NAME} = ${VALUE};"
        ;;
      *ALIAS*)
        addline "/**"
        addline " * @alias ${ALIAS}"
        addline " */"

        addline "${STATIC:+$STATIC }get ${NAME}() { return this.${ALIAS}; }"
        ;;
      *)
        echo "Macro: $MACRO" 1>&2
        exit 1
        ;;
      esac

      ;;

    *) continue ;;
    esac

    if [ ${#O} -gt 0 ]; then
      echo "$O"
      O=
    fi

  done <"$1"
}

[ $# -le 0 ] && set -- js_*.cpp

for ARG; do
  OUT=${ARG#js_}
  OUT=${OUT%.cpp}.js
  echo "Generating $OUT from $ARG ..." 1>&2
  process_proto "$ARG" >$OUT
done
