LDFLAGS="-pg"  CFLAGS="-g3 -ggdb -w -pg" CXXFLAGS="-g3 -ggdb -w -pg" prefix=/usr/local TYPE=RelWithDebInfo builddir=build/x86_64-linux-profile  cfg -DOpenCV_DIR=/opt/opencv-4.7.0-x86_64/lib/cmake/opencv4
prefix=/usr/local TYPE=Release  cfg  -DOpenCV_DIR=/opt/opencv-4.7.0-x86_64/lib/cmake/opencv4
CFLAGS="-g3 -ggdb -O0"  CXXFLAGS="-g3 -ggdb -O0"  prefix=/usr/local TYPE=Debug builddir=build/x86_64-linux-debug  cfg   -DOpenCV_DIR=/opt/opencv-4.7.0-x86_64/lib/cmake/opencv4
