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

# CUDA/cuDNN options, same as cfg-cuda in ../opencv/cfg.sh. CUDA=ON|OFF
# forces it, by default it is used when nvcc and cuDNN are found.
cuda_opencv() {
  : ${CUDNN_INCLUDE_DIR:=/usr/include/x86_64-linux-gnu}
  : ${CUDNN_LIBRARY:=/usr/lib/x86_64-linux-gnu/libcudnn.so}

  if [ -z "$CUDA" ]; then
    if type nvcc >/dev/null 2>&1 && [ -e "$CUDNN_LIBRARY" ]; then CUDA=ON; else CUDA=OFF; fi
  fi
  case "$CUDA" in
  ON | on | YES | yes | 1) ;;
  *) return 0 ;;
  esac

  # nvcc 12.0 only accepts host compilers up to gcc 12
  : ${CUDA_HOST_COMPILER:=$(type -p gcc-12 || echo /usr/bin/gcc)}
  # compute capability of the GPU: nvidia-smi --query-gpu=compute_cap --format=csv
  : ${CUDA_ARCH_BIN:=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader 2>/dev/null | head -n1)}
  : ${CUDA_ARCH_BIN:=7.5}

  echo -DWITH_CUDA=ON -DWITH_CUDNN=ON -DOPENCV_DNN_CUDA=ON -DWITH_CUBLAS=ON \
    -DCUDA_HOST_COMPILER="$CUDA_HOST_COMPILER" \
    -DCMAKE_CUDA_HOST_COMPILER="$CUDA_HOST_COMPILER" \
    -DCUDNN_INCLUDE_DIR="$CUDNN_INCLUDE_DIR" \
    -DCUDNN_LIBRARY="$CUDNN_LIBRARY" \
    -DCUDA_ARCH_BIN="$CUDA_ARCH_BIN" \
    -DWITH_NVCUVID="${WITH_NVCUVID:-$([ -e /usr/include/nvcuvid.h ] && echo ON || echo OFF)}" \
    -DWITH_NVCUVENC="${WITH_NVCUVENC:-$([ -e /usr/include/nvEncodeAPI.h ] && echo ON || echo OFF)}"
}

configure_opencv() {
  : ${CC=cc}
  : ${host=$($CC -dumpmachine)}
  
  download_opencv

  : ${builddir=$sourcedir/build/$host}  

  prepare_opencv

  cuda_args=$(cuda_opencv)

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
    $cuda_args \
    "$@") 2>&1 | tee cmake.log
}


build_opencv() {
 (configure_opencv "$@"
  
  (make -C "$builddir" -j${JOBS:-$(nproc)} 2>&1 | tee build.log))
}
