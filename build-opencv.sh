download_opencv() {
  : ${sourcedir=opencv}
  : ${version=4.5.5}

  if [ -d "${sourcedir}" -a '!' -d "${sourcedir}/modules" ]; then
    rm -rf "${sourcedir}/modules"
  fi
  if [ ! -d "${sourcedir}" ]; then
    (set -x; git clone ${version:+-b"$version"} https://github.com/opencv/opencv.git "${sourcedir}")
  fi
  if [ -d "${sourcedir}_contrib" -a '!' -d "${sourcedir}_contrib/modules" ]; then
    rm -rf "${sourcedir}_contrib/modules"
  fi
  if [ ! -d "${sourcedir}_contrib" ]; then
    (set -x; git clone ${version:+-b"$version"} https://github.com/opencv/opencv_contrib.git "${sourcedir}_contrib")
  fi
}


prepare_opencv() {
  mkdir -p "$builddir"
  echo "builddir: $builddir" 1>&2

  relsrcdir=$(realpath --relative-to "$builddir" "$sourcedir")

  if [ -z "$EXTRA_MODULES" ]; then
    if [ -d "${sourcedir}_contrib/modules" ]; then
      EXTRA_MODULES=$(realpath "${sourcedir}_contrib/modules")
    fi
  fi
}

configure_opencv() {
  : ${CC=cc}
  : ${host=$($CC -dumpmachine)}
  
  download_opencv

  : ${builddir=$sourcedir/build/$host}  

  prepare_opencv

  (cd $builddir
    cmake $relsrcdir \
    ${TOOLCHAIN+"-DCMAKE_TOOLCHAIN_FILE:FILEPATH=$TOOLCHAIN"} \
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=${VERBOSE-OFF} \
    -DCMAKE_BUILD_TYPE="${TYPE:-RelWithDebInfo}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DBUILD_SHARED_LIBS=ON \
    ${CC:+-DCMAKE_C_COMPILER="$CC"} \
    ${CXX:+-DCMAKE_CXX_COMPILER="$CXX"} \
    ${PKG_CONFIG:+-DPKG_CONFIG_EXECUTABLE="$PKG_CONFIG"} \
    ${EXTRA_MODULES+-DOPENCV_EXTRA_MODULES_PATH="$EXTRA_MODULES"} \
    -DOPENCV_GENERATE_PKGCONFIG=ON \
    -DOPENCV_ENABLE_NONFREE=ON \
    -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DWITH_{EIGEN,FREETYPE,GPHOTO2,OPENGL,TBB,TESSERACT,VA,VA_INTEL}=ON \
    "$@") 2>&1 | tee cmake.log
}


build_opencv() {
 (configure_opencv "$@"
  
  (make -C "$builddir" 2>&1 | tee build.log))
}
