#!/bin/sh
NL="
"
while read -r LINE; do
  case "$LINE" in
  *".class_name = "*)
    CLASS=${LINE##*"="}
    CLASS=${CLASS%"\""*}
    CLASS=${CLASS#*"\""}
    O="class $CLASS {${NL}  constructor() {}"
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
    *static*) STATIC='static' ;;
    *) STATIC='' ;;
    esac
    continue
    ;;
  *"_DEF("*)
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
    O="  ${STATIC:+$STATIC }"
    case $MACRO in
    *GETSET*)
      O="${O}get ${NAME}() {}"
      ;;
    *FUNC*)
      O="${O}${NAME}() {}"
      ;;
    *INT32* | *STRING*) O="${O}${NAME} = $1;" ;;
    *ALIAS*) O="${O}get ${NAME}() { return this.${ALIAS}; }" ;;
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
