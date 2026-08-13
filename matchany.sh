matchany () 
{ 
    ( STR="$1";
    shift;
    : set -o noglob;
    for EXPR in "$@";
    do
        case "$STR" in 
            *$EXPR*)
                exit 0
            ;;
            *)

            ;;
        esac;
    done;
    exit 1 )
}
