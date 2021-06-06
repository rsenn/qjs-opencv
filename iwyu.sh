#!/bin/sh
MYDIR=`dirname "$0"`

cd "$MYDIR"
set -- *.?pp
echo "Processing $# files..." 1>&2
for x; do
  (set -x; include-what-you-use -Xiwyu --quoted_includes_first -w -std=c++20 -I /opt/opencv-4.5.0/include/opencv4 -I ../quickjs "$x")
done 2>&1 | tee iwyu.txt
