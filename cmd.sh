. cfg.sh
set -- \
	-DOpenCV_DIR=/opt/opencv-4.7.0-x86_64/lib/cmake/opencv4 \
	-DOPENCV_PREFIX=/opt/opencv-4.7.0-x86_64 \
	-DOPENCV_LIBDIR=/opt/opencv-4.7.0-x86_64/lib

TYPE=Release prefix=/usr/local cfg "$@"

CFLAGS="-g3 -ggdb -O0 -Wall" prefix=/usr/local TYPE=Debug builddir=build/x86_64-linux-debug cfg "$@"

LDFLAGS="-pg" CFLAGS="-g3 -ggdb -w -pg" CXXFLAGS="-g3 -ggdb -w -pg" prefix=/usr/local TYPE=RelWithDebInfo builddir=build/x86_64-linux-profile cfg "$@"

CC=clang CXX=clang++ prefix=/usr/local TYPE=RelWithDebInfo builddir=build/x86_64-linux-clang cfg "$@"
