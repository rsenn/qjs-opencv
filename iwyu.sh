for x in *.?pp; do include-what-you-use -Xiwyu --quoted_includes_first -std=c++20 -I /opt/opencv-4.5.0/include/opencv4 -I ../quickjs "$x"; done 2>&1 | tee iwyu.txt
