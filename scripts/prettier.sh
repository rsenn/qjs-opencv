#!/bin/sh

ME=${0}
MYNAME=`basename "${ME%.sh}"`

if ! type prettier 2>/dev/null >/dev/null; then
  for DIR in $HOME/.nvm/versions/node/*/bin; do
    PATH="$PATH:$DIR"
  done
fi

prettier() {
 ( set -- ${PRETTIER:-prettier} \
    $OPTS \
    --parser ${PARSER:-babel} \
    --jsx-single-quote \
    --trailing-comma none \
    --write \
    --print-width ${WIDTH:-120} \
    --semi \
    --arrow-parens avoid \
    --bracket-spacing \
    ${CONFIG:+--config
"$CONFIG"} \
    --no-insert-pragma \
    "$@"; ${DEBUG:-false} && echo "$@" 1>&2; command "$@" 2>&1 ##| grep -v 'ExperimentalWarning:'
 exit $?)
}

main() {
  PRE_EXPR='\|/\*| { :lp; \|\*/|! { N; b lp }; n }'
  #PRE_EXPR='/^\s*[sg]et\s[^\s\t ]\+\s*([^)]*)\s*{/ s|^\(\s*\)|\1/* prettier-ignore */ |'
  PRE_EXPR="$PRE_EXPR;; "'/^\s*[sg]et\s\+/ s|^\(\s*\)|\1/* prettier-ignore */ |'
  POST_EXPR='1 { /@format/ { N; /\n$/ { d } } }'
  for KW in "if" "for" "for await" "do" "while" "catch" "function"; do
    POST_EXPR="$POST_EXPR; s|\s${KW}\s*(| ${KW}(|"
    POST_EXPR="$POST_EXPR; s|^${KW}\s*(|${KW}(|"
  done
  POST_EXPR="$POST_EXPR; /($/ { N; s|(\n\s*|(| }"
  POST_EXPR="$POST_EXPR; /([^,]*,$/ { N; s|^\(\s*[^ ]*([^,]*,\)\n\s*{|\1 {| }"
  POST_EXPR="$POST_EXPR; /^\s*[^ ]\+:$/ { N; s|^\(\s*[^ ]\+:\)\n\s*|\1 | }"
  POST_EXPR="$POST_EXPR; /^\s*},$/  { N; s|^\(\s*},\)\n\s*\[|\1 [| }"
  #POST_EXPR="$POST_EXPR; /^\s*let\s/ { :lp; /;\s*$/! { N; s,\s*\n\s*, ,g; b lp } }"
  #POST_EXPR="$POST_EXPR; /^\s*const\s/ { :lp; /;\s*$/! { N; s,\s*\n\s*, ,g; b lp } }"
  #POST_EXPR="$POST_EXPR; /^\s*var\s/ { :lp; /;\s*$/! { N; s,\s*\n\s*, ,g; b lp } }"
  POST_EXPR="$POST_EXPR; /^import/ { :lp; /;$/! { N; b lp };  s|\n\s*| |g }"
  POST_EXPR="$POST_EXPR; /\*\/\s[gs]et\s/ s|/\* prettier-ignore \*/ ||g"
  #POST_EXPR="$POST_EXPR; "':st; /^\s*[gs]et(/ { N; /\n\s*[^{}\n]*$/  N;  /\/\/.*\n/! { /\n\s*},$/ s,\n\s*, ,g } }'

  SEP=${IFS%"${IFS#?}"}

  [ -f .prettierrc ] && CONFIG=.prettierrc

  while [ $# -gt 0 ]; do
    case "$1" in
      --parser=*) PARSER=${1#*=}; shift ;; --parser) PARSER=$2; shift 2 ;;
      -*) OPTS="${OPTS:+$OPTS$SEP}$1"; shift ;;
      *) break ;;
    esac
  done

  if [ $# -le 0 ]; then
    set -- $(find . -maxdepth 1 -type f -name "*.js"; find lib -type f -name "*.js")
    set -- "${@#./}"
    set -- $(ls -td -- "$@")
  fi

  for SOURCE; do
    case "$SOURCE" in
      *.es5.js) continue ;;
    esac
    [ -e "$SOURCE" -a ! -f "$SOURCE" ] && continue
    ARG=${SOURCE//"["/"\\["}
    ARG=${ARG//"]"/"\\]"}
   ( trap 'rm -f "$TMPFILE" "$DIFFFILE"' EXIT
   DIR=`dirname "$ARG"`
    TMPFILE=`mktemp --tmpdir "$MYNAME-XXXXXX.tmp"`
    DIFFFILE=`mktemp --tmpdir "$MYNAME-XXXXXX.diff"`
    echo "Processing ${SOURCE} ..." 1>&2
    case "$SOURCE" in
      *.css) : ${PARSER="css"} ;;
      *) ;;
    esac
    sed <"$ARG" "$PRE_EXPR" | prettier >"$TMPFILE"; R=$?

    if [ $R != 0 ]; then
      cat "$TMPFILE" 1>&2 
      exit 1
     fi
    (: set -x; sed -i  "$POST_EXPR" "$TMPFILE") &&
    {  diff -u "$ARG" "$TMPFILE" | sed "s|$TMPFILE|$ARG|g; s|$ARG|${ARG##*/}|" | tee "$DIFFFILE" |  diffstat |sed "1! d; /0 files/d"  ;  (cd "$DIR" && patch -p0 ) <"$DIFFFILE" && rm -f "$TMPFILE" || mv -vf "$TMPFILE" "$ARG"; }) || {
      echo "SOURCE=$SOURCE" 1>&2
      return $?
  }
   done
}

main "$@"

