#!/bin/bash
# build-opencv.sh - build OpenCV (+ opencv_contrib) for qjs-opencv
#
#   ./build-opencv.sh [-n] TARGET [STEP...]
#
# TARGET   host           this machine (Ubuntu desktop, x86_64 glibc), native
#          pi             Raspberry Pi 4, aarch64 glibc, native: run it ON the Pi
#          termux         Termux, aarch64 bionic, native: run it ON the phone
#          pi-cross       the Pi, cross build on the workstation (clang + lld + ~/sysroots/pi)
#          termux-cross   Termux, cross build on the workstation (NDK + ~/sysroots/termux/usr)
# STEP     configure (default), build, install, deploy (cross only), smoke (cross only), where
#          'configure' also prints the "To be built:" module list.
# -n       print the cmake command instead of running it
#
# The functions download_opencv, prepare_opencv, cuda_opencv, configure_opencv and
# build_opencv can still be sourced and used on their own (. ./build-opencv.sh).
#
# Environment (all optional):
#   version        tag to clone when sourcedir does not exist yet (default 4.13.0)
#   sourcedir      OpenCV checkout. Never modified when it exists (no checkout, no pull).
#                  Default: ~/Sources/opencv on the host when present, else a clone of
#                  $version next to the work dir. opencv_contrib: ${sourcedir}_contrib.
#   PROFILE        full: the reference build (~/Sources/opencv/cfg.sh: GTK, FFmpeg, GStreamer,
#                  CUDA when found, all modules); min: only the modules qjs-opencv links, no
#                  tests/docs/java/python/CUDA. Default full for 'host', min for the rest.
#   BUILD_LIST     overrides the module list of the min profile
#   TYPE           CMAKE_BUILD_TYPE, default Release
#   JOBS           parallel jobs, default min(cpus, free RAM / 1.2 GB)
#   CMAKE          cmake to use (default: newest of cmake and /opt/cmake-*/bin/cmake)
#   prefix         install prefix on the device (see 'where')
#   STAGE          DESTDIR of a cross install (default /opt/opencv-<ver>-aarch64[-termux]-stage)
#   builddir       default ./build/opencv-<ver>-<target>
#   FORCE=1        install/deploy into a prefix that already exists
#   TARGETS        build only these cmake targets (e.g. "opencv_core opencv_imgproc")
#   CMAKE_ARGS     extra cmake options, e.g. CMAKE_ARGS='-DWITH_ONNX=ON -DORT_INSTALL_DIR=...'
#   ORT_VERSION    ONNX Runtime version, all targets (default 1.32.0), see fetch_onnxruntime
#   OPENCV_DOWNLOAD_PATH  where OpenCV looks for downloads (default <sourcedir>/.cache)
#   PI_SYSROOT, TERMUX_SYSROOT, NDK, ANDROID_PLATFORM, PI_HOST, TERMUX_HOST, PI_DEPLOY

# Note: to change options of an existing build dir delete its CMakeCache.txt, do not sed-edit it
# (orphaned comment lines break the cache parse).

# ONNX Runtime. OpenCV's own download from github.com/microsoft/onnxruntime releases fails from some
# networks (404) and has no Android package, so the NuGet package microsoft.ml.onnxruntime is repacked
# into the tgz OpenCV's DOWNLOAD_ONNXRUNTIME expects (top dir = name without .tgz, with include/ lib/).
# target -> (file in the nupkg, cache name):
#   host x86_64   runtimes/linux-x64/native/*.so          onnxruntime-linux-x64-<ver>.tgz
#   host aarch64  runtimes/linux-arm64/native/*.so        onnxruntime-linux-aarch64-<ver>.tgz
#   pi, pi-cross  runtimes/linux-arm64/native/*.so        onnxruntime-linux-aarch64-<ver>.tgz
#   termux, -cross runtimes/android/native/onnxruntime.aar, its jni/arm64-v8a/libonnxruntime.so
#                 onnxruntime-android-arm64-<ver>.tgz: OpenCV has no download for it, so it is unpacked
#                 next to the tgz and handed over as ONNXRT_ROOT_DIR (+ HAVE_ONNXRUNTIME=1, see bo_ort_args).
#                 Only configure-tested: not verified that it loads on a phone (Bionic; the .so needs
#                 libdl liblog libandroid libm libc, no libc++_shared)
# other arch / target: no package, ORT is skipped with a log line. The nupkg (~180 MB) stays in the cache.
# Version: ORT_VERSION (default 1.32.0). Cache: OPENCV_DOWNLOAD_PATH (default <sourcedir>/.cache).
bo_ort_target() {
  ort_ver=${ORT_VERSION:-1.32.0}
  ort_dl=${OPENCV_DOWNLOAD_PATH:-$(cd "$sourcedir" && pwd)/.cache}/onnxruntime
  ort_aar=
  case $base:$arch in
  host:x86_64) ort_name=onnxruntime-linux-x64-$ort_ver ort_rt=linux-x64 ;;
  host:aarch64 | pi:*) ort_name=onnxruntime-linux-aarch64-$ort_ver ort_rt=linux-arm64 ;;
  termux:*) ort_name=onnxruntime-android-arm64-$ort_ver ort_rt=android ort_aar=1 ;;
  *) ort_name= ;;
  esac
  [ -n "$ort_name" ]
}

# cmake options for ONNX Runtime
bo_ort_args() {
  bo_ort_target || return 0
  if [ -n "$ort_aar" ]; then
    # not downloadable by OpenCV (dnn/CMakeLists.txt only knows win/apple/linux x64+aarch64, and would fetch
    # the glibc package for an NDK build): HAVE_ONNXRUNTIME=1 on the command line keeps it from trying,
    # FindONNX then finds lib/ and include/ under ONNXRT_ROOT_DIR
    echo -DWITH_ONNXRUNTIME=ON -DHAVE_ONNXRUNTIME=1 -DONNXRT_ROOT_DIR="$ort_dl/$ort_name"
  else
    # not -DONNXRT_ROOT_DIR: the download branch overwrites it
    echo -DWITH_ONNXRUNTIME=ON -DDOWNLOAD_ONNXRUNTIME=ON -DONNXRUNTIME_VERSION="$ort_ver"
  fi
}

fetch_onnxruntime() {
  local nupkg tgz tmp rc=0 f
  bo_ort_target || { bo_log "ONNX Runtime: no package for $base/$arch, skipped"; return 0; }
  tgz=$ort_dl/$ort_name.tgz
  nupkg=$ort_dl/microsoft.ml.onnxruntime.$ort_ver.nupkg
  if [ -f "$tgz" ] && { [ -z "$ort_aar" ] || [ -d "$ort_dl/$ort_name/lib" ]; }; then return 0; fi
  if [ -n "$DRYRUN" ]; then echo "(fetch $tgz from NuGet microsoft.ml.onnxruntime $ort_ver)"; return 0; fi
  mkdir -p "$ort_dl" || return 1
  tmp=$(mktemp -d) || return 1
  bo_log "ONNX Runtime $ort_ver ($ort_rt) -> $tgz"
  mkdir -p "${tmp:?}/x" "${tmp:?}/$ort_name/include" "${tmp:?}/$ort_name/lib"
  if [ ! -f "$tgz" ]; then
    if { [ -f "$nupkg" ] || { curl -fL --retry 2 -o "$nupkg.part" \
      "https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime/$ort_ver/microsoft.ml.onnxruntime.$ort_ver.nupkg" &&
      mv "$nupkg.part" "$nupkg"; }; } &&
      unzip -q -o "$nupkg" 'build/native/include/*' "runtimes/$ort_rt/native/*" -d "${tmp:?}/x" &&
      cp -a "${tmp:?}"/x/build/native/include/. "${tmp:?}/$ort_name/include/" &&
      if [ -n "$ort_aar" ]; then
        unzip -q -o "${tmp:?}/x/runtimes/android/native/onnxruntime.aar" jni/arm64-v8a/libonnxruntime.so -d "${tmp:?}/aar" &&
          cp -a "${tmp:?}/aar/jni/arm64-v8a/libonnxruntime.so" "${tmp:?}/$ort_name/lib/"
      else
        cp -a "${tmp:?}"/x/runtimes/$ort_rt/native/*.so "${tmp:?}/$ort_name/lib/"
      fi &&
      tar -C "${tmp:?}" -czf "${tmp:?}/out.tgz" "$ort_name" &&
      mv "${tmp:?}/out.tgz" "$tgz"; then :; else rc=1; bo_log "ONNX Runtime fetch FAILED"; fi
  fi
  # the Android one is not downloaded by OpenCV: unpack it where ONNXRT_ROOT_DIR points
  [ $rc = 0 ] && [ -n "$ort_aar" ] && { tar -C "$ort_dl" -xzf "$tgz" || rc=1; }
  rm -rf "${tmp:?}"
  return $rc
}

download_opencv() {
  : ${sourcedir=opencv}
  : ${version=4.13.0}

  if [ -d "${sourcedir}" -a '!' -d "${sourcedir}/modules" ]; then
    rm -rf "${sourcedir}/modules"
  fi
  if [ ! -d "${sourcedir}" ]; then
    (set -x; git clone --depth 1 ${version:+-b"$version"} https://github.com/opencv/opencv.git "${sourcedir}") || return 1
  fi
  if [ -d "${sourcedir}_contrib" -a '!' -d "${sourcedir}_contrib/modules" ]; then
    rm -rf "${sourcedir}_contrib/modules"
  fi
  if [ ! -d "${sourcedir}_contrib" ]; then
    (set -x; git clone --depth 1 ${version:+-b"$version"} https://github.com/opencv/opencv_contrib.git "${sourcedir}_contrib") || return 1
  fi
}

prepare_opencv() {
  mkdir -p "$builddir"
  echo "builddir: $builddir" 1>&2

  # absolute: realpath --relative-to gave a wrong path on Termux (/data/data vs /data/user/0)
  relsrcdir=$(cd "$sourcedir" && pwd)

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

# ----------------------------------------------------------------------------
# helpers

bo_log() { echo "build-opencv: $*" 1>&2; }

# cmake: the default one when it is recent enough (3.20), else the newest in /opt/cmake-*/bin
# (the Pi's default is 3.18.4). Not simply the newest: OpenCV 4.x predates cmake 4's removal
# of the old policies.
bo_pick_cmake() {
  [ -n "$CMAKE" ] && return 0
  d=$(command -v cmake 2>/dev/null)
  if [ -n "$d" ]; then
    v=$("$d" --version | sed -n '1s/[^0-9]*\([0-9][0-9.]*\).*/\1/p')
    if [ "$(printf '3.20\n%s\n' "$v" | sort -V | head -n1)" = 3.20 ]; then CMAKE=$d; return 0; fi
  fi
  best=
  bestv=0
  for c in /opt/cmake-*/bin/cmake; do
    [ -x "$c" ] || continue
    v=$("$c" --version 2>/dev/null | sed -n '1s/[^0-9]*\([0-9][0-9.]*\).*/\1/p')
    [ -n "$v" ] || continue
    if [ "$(printf '%s\n%s\n' "$bestv" "$v" | sort -V | tail -n1)" = "$v" ]; then best=$c bestv=$v; fi
  done
  [ -n "$best" ] || best=$d
  [ -n "$best" ] || { bo_log "no cmake found"; return 1; }
  CMAKE=$best
  export PATH=$(dirname "$CMAKE"):$PATH
}

# parallel jobs from the CPU count and the free RAM (about 1.2 GB per C++ compile of OpenCV)
bo_jobs() {
  [ -n "$JOBS" ] && return 0
  cpus=$(nproc 2>/dev/null || echo 2)
  kb=$(sed -n 's/^MemAvailable: *\([0-9]*\).*/\1/p' /proc/meminfo 2>/dev/null)
  if [ -n "$kb" ]; then
    j=$((kb / 1200000))
    [ "$j" -lt 1 ] && j=1
    [ "$j" -lt "$cpus" ] && cpus=$j
  fi
  JOBS=$cpus
}

# CV_VERSION_* of the checkout, e.g. 5.1.0
bo_source_version() {
  h=$sourcedir/modules/core/include/opencv2/core/version.hpp
  [ -r "$h" ] || { echo "${version:-unknown}"; return; }
  maj=$(sed -n 's/^#define CV_VERSION_MAJOR *\([0-9]*\).*/\1/p' "$h")
  min=$(sed -n 's/^#define CV_VERSION_MINOR *\([0-9]*\).*/\1/p' "$h")
  rev=$(sed -n 's/^#define CV_VERSION_REVISION *\([0-9]*\).*/\1/p' "$h")
  echo "$maj.$min.$rev"
}

# The modules qjs-opencv really links (CMakeLists.txt and the #includes of js_*.cpp):
# core imgproc imgcodecs videoio highgui calib3d/calib features2d/features flann dnn
# objdetect photo video, and from opencv_contrib ximgproc xfeatures2d xphoto bgsegm
# freetype alphamat. 4.x and 5.x name some modules differently (features2d/features,
# calib3d/calib, ...); BUILD_LIST ignores names that do not exist, the dependencies
# of the listed modules are added by OpenCV itself.
BO_MIN_LIST=core,imgproc,imgcodecs,videoio,highgui,calib3d,calib,3d,features2d,features,geometry,flann,dnn,objdetect,photo,video,stereo,ximgproc,xfeatures2d,xphoto,bgsegm,freetype,alphamat

# ----------------------------------------------------------------------------
# target setup: sets native, base, arch, prefix, stage, builddir, target_args, cross_* ...

bo_setup_target() {
  target=$1
  case $target in
  host | pi | termux | pi-cross | termux-cross) ;;
  *) bo_log "unknown target '$target' (host pi termux pi-cross termux-cross)"; return 1 ;;
  esac
  native=true
  case $target in *-cross) native=false ;; esac
  base=${target%-cross}

  # sources
  if [ -z "$sourcedir" ]; then
    if [ "$base" = host ] && [ -d "$HOME/Sources/opencv/modules" ]; then
      sourcedir=$HOME/Sources/opencv
    elif [ "$base" = pi ] && $native; then
      sourcedir=$HOME/ci-work/opencv-${version:-4.13.0}
    else
      sourcedir=$PWD/../opencv-${version:-4.13.0}
    fi
  fi
  download_opencv || return 1
  sourcedir=$(realpath "$sourcedir")
  ver=$(bo_source_version)
  major=${ver%%.*}

  case $base in
  host)
    arch=$(uname -m)
    osname=
    : ${PROFILE:=full}
    : ${prefix:=/opt/opencv-$ver-$arch}
    ;;
  pi)
    arch=aarch64
    osname=
    : ${PROFILE:=min}
    # on the Pi itself the prefix is the one the existing /opt tree uses
    : ${prefix:=/opt/opencv-$ver-aarch64}
    if $native; then
      [ "$(uname -m)" = aarch64 ] || { bo_log "target pi runs on the Pi (aarch64), use pi-cross here"; return 1; }
    fi
    ;;
  termux)
    arch=aarch64
    osname=-termux
    : ${PROFILE:=min}
    : ${TERMUX_PREFIX:=/data/data/com.termux/files/usr}
    # next to, never over, the OpenCV package that lives in the Termux prefix itself
    : ${prefix:=$TERMUX_PREFIX/opt/opencv-$ver}
    if $native; then
      case $(uname -a)$PREFIX in *ndroid* | */com.termux/*) ;; *) bo_log "target termux runs on the phone, use termux-cross here"; return 1 ;; esac
    fi
    ;;
  esac
  : ${STAGE:=/opt/opencv-$ver-$arch$osname-stage}
  : ${builddir:=$PWD/build/opencv-$ver-$target}
  : ${TYPE:=Release}

  target_args=()
  cross_cxx=
  cross_flags=
  stage_prefix=$prefix
  $native || stage_prefix=$STAGE$prefix

  if ! $native; then
    mkdir -p "$builddir"
    bd=$(cd "$builddir" && pwd)
    case $base in
    pi) bo_setup_pi_cross ;;
    termux) bo_setup_termux_cross ;;
    esac || return 1
  fi
  return 0
}

# cfg-aarch64 of c-utils/cfg.sh: clang + lld with --target and --sysroot, no cross gcc needed
bo_setup_pi_cross() {
  : ${PI_SYSROOT:=$HOME/sysroots/pi}
  : ${PI_TARGET:=aarch64-linux-gnu}
  [ -d "$PI_SYSROOT/usr/lib" ] || { bo_log "no Pi sysroot at $PI_SYSROOT (make sysroot-pi in js-engines)"; return 1; }
  command -v clang >/dev/null && command -v ld.lld >/dev/null || { bo_log "needs clang and lld"; return 1; }
  cat >"$bd/pi-toolchain.cmake" <<EOF
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_SYSROOT $PI_SYSROOT)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET $PI_TARGET)
set(CMAKE_CXX_COMPILER_TARGET $PI_TARGET)
set(CMAKE_ASM_COMPILER_TARGET $PI_TARGET)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_FIND_ROOT_PATH $PI_SYSROOT)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
EOF
  cat >"$bd/pi-pkg-config" <<EOF
#!/bin/sh
PKG_CONFIG_SYSROOT_DIR=$PI_SYSROOT
PKG_CONFIG_LIBDIR=$PI_SYSROOT/usr/lib/$PI_TARGET/pkgconfig:$PI_SYSROOT/usr/lib/pkgconfig:$PI_SYSROOT/usr/share/pkgconfig
unset PKG_CONFIG_PATH
export PKG_CONFIG_SYSROOT_DIR PKG_CONFIG_LIBDIR
exec pkg-config "\$@"
EOF
  chmod +x "$bd/pi-pkg-config"
  TOOLCHAIN=$bd/pi-toolchain.cmake
  PKG_CONFIG=$bd/pi-pkg-config
  cross_cxx=clang++
  cross_flags="--target=$PI_TARGET --sysroot=$PI_SYSROOT -fuse-ld=lld"
  # OpenCV ignores pkg-config when cross compiling unless told; the wrapper points it at the sysroot
  target_args=(-DCMAKE_TOOLCHAIN_FILE:FILEPATH=$TOOLCHAIN -DOPENCV_ENABLE_PKG_CONFIG=ON)
}

# cfg-termux of c-utils/cfg.sh: the NDK toolchain file plus the Termux packages' headers and libs.
# The Android toolchain makes OpenCV use its SDK layout (sdk/native/...), so the unix layout is
# set explicitly: lib/, include/opencv<major>, lib/cmake/opencv<major>.
bo_setup_termux_cross() {
  : ${NDK:=/mnt/data/opt/android-ndk-r27c}
  : ${TERMUX_SYSROOT:=$HOME/sysroots/termux/usr}
  : ${ANDROID_ABI:=arm64-v8a}
  : ${ANDROID_PLATFORM:=android-24}
  TOOLCHAIN=$NDK/build/cmake/android.toolchain.cmake
  [ -e "$TOOLCHAIN" ] || { bo_log "no $TOOLCHAIN (set NDK)"; return 1; }
  [ -d "$TERMUX_SYSROOT/lib" ] || { bo_log "no Termux sysroot at $TERMUX_SYSROOT (make sysroot-termux in js-engines)"; return 1; }
  cat >"$bd/termux-pkg-config" <<EOF
#!/bin/sh
PKG_CONFIG_PATH=$TERMUX_SYSROOT/lib/pkgconfig:$TERMUX_SYSROOT/share/pkgconfig
export PKG_CONFIG_PATH
out=\$(pkgcfg --path-map $TERMUX_PREFIX=$TERMUX_SYSROOT "\$@") || exit
printf '%s\n' "\$out" | sed -E 's#-I$TERMUX_SYSROOT/include( |\$)#-idirafter $TERMUX_SYSROOT/include\1#g'
EOF
  chmod +x "$bd/termux-pkg-config"
  PKG_CONFIG=$bd/termux-pkg-config
  api=${ANDROID_PLATFORM#android-}
  cross_cxx=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$api-clang++
  cross_flags="-idirafter $TERMUX_SYSROOT/include -L$TERMUX_SYSROOT/lib"
  c=opencv$major
  target_args=(
    -DCMAKE_TOOLCHAIN_FILE:FILEPATH=$TOOLCHAIN -DANDROID_ABI=$ANDROID_ABI -DANDROID_PLATFORM=$ANDROID_PLATFORM
    -DANDROID_STL=c++_shared -DOPENCV_ENABLE_PKG_CONFIG=ON
    -DCMAKE_FIND_ROOT_PATH=$TERMUX_SYSROOT -DCMAKE_PREFIX_PATH=$TERMUX_SYSROOT
    "-DCMAKE_C_FLAGS=-idirafter $TERMUX_SYSROOT/include" "-DCMAKE_CXX_FLAGS=-idirafter $TERMUX_SYSROOT/include"
    "-DCMAKE_EXE_LINKER_FLAGS=-L$TERMUX_SYSROOT/lib -Wl,-rpath,$TERMUX_PREFIX/lib"
    "-DCMAKE_SHARED_LINKER_FLAGS=-L$TERMUX_SYSROOT/lib -Wl,-rpath,$TERMUX_PREFIX/lib"
    -DOPENCV_BIN_INSTALL_PATH=bin -DOPENCV_LIB_INSTALL_PATH=lib -DOPENCV_LIB_ARCHIVE_INSTALL_PATH=lib
    -DOPENCV_3P_LIB_INSTALL_PATH=lib/$c/3rdparty -DOPENCV_CONFIG_INSTALL_PATH=lib/cmake/$c
    -DOPENCV_INCLUDE_INSTALL_PATH=include/$c -DOPENCV_OTHER_INSTALL_PATH=share/$c
    -DOPENCV_LICENSES_INSTALL_PATH=share/licenses/$c -DOPENCV_TEST_DATA_INSTALL_PATH=share/$c/testdata
    -DOPENCV_SAMPLES_SRC_INSTALL_PATH=share/$c/samples -DOPENCV_SAMPLES_BIN_INSTALL_PATH=bin
    -DOPENCV_TEST_INSTALL_PATH=bin -DOPENCV_DOC_INSTALL_PATH=share/doc/$c
  )
}

# ----------------------------------------------------------------------------
# configure / build / install

# profile dependent options, one per line
bo_profile_args() {
  case $PROFILE in
  full)
    # the reference build: ~/Sources/opencv/cfg.sh and build/x86_64-linux-gnu/CMakeCache.txt
    echo -DWITH_{EIGEN,FREETYPE,GPHOTO2,OPENGL,TBB,TESSERACT,VA,VA_INTEL}=ON
    echo -DBUILD_TESTS=${TESTS:-OFF} -DBUILD_PERF_TESTS=${TESTS:-OFF}
    case $arch in
    x86_64) echo -DCPU_BASELINE=SSE3 "-DCPU_DISPATCH=SSE4_1;SSE4_2;AVX;FP16;AVX2;AVX_VNNI;AVX512_SKX" ;;
    esac
    echo -DWITH_FFMPEG=ON -DWITH_GTK=ON -DWITH_GSTREAMER=ON -DWITH_QT=OFF -DWITH_V4L=ON
    # the Pi's TBB 2020.3 headers (tbb/task.h: kind_type(bound+1) out of the enum's range) are a hard error for
    # clang >= 16 and no warning flag changes that: the clang cross build uses OpenCV's pthreads backend instead
    [ "$base" = pi ] && ! $native && echo -DWITH_TBB=OFF
    $native && cuda_opencv
    ;;
  min)
    echo -DBUILD_LIST="${BUILD_LIST:-$BO_MIN_LIST}"
    echo -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_DOCS=OFF -DBUILD_EXAMPLES=OFF
    echo -DBUILD_JAVA=OFF -DBUILD_opencv_java=OFF -DBUILD_opencv_java_bindings_generator=OFF
    echo -DBUILD_opencv_python2=OFF -DBUILD_opencv_python3=OFF -DBUILD_opencv_python_bindings_generator=OFF
    echo -DBUILD_opencv_apps=OFF -DBUILD_opencv_js=OFF
    echo -DWITH_CUDA=OFF -DWITH_EIGEN=ON -DWITH_FREETYPE=ON
    echo -DWITH_{GPHOTO2,OPENGL,TBB,TESSERACT,VA,VA_INTEL,GSTREAMER,QT}=OFF
    # the Termux sysroot's cmake package configs name device paths that do not exist on the host
    # (libavif-config.cmake aborts the configure), so those packages are skipped
    [ "$base" = termux ] && echo -DWITH_GTK=OFF -DWITH_AVIF=OFF -DBUILD_ANDROID_PROJECTS=OFF -DBUILD_ANDROID_EXAMPLES=OFF -DBUILD_ANDROID_SERVICE=OFF
    ;;
  *) bo_log "PROFILE must be full or min"; return 1 ;;
  esac
  # the Pi, both profiles: its system libavif is 0.8.4, too old for grfmt_avif.cpp (avifDecoder::diag and
  # strictFlags need a newer one), so no AVIF; in the cross build its OpenJPEG cmake config names files that
  # are not in the sysroot copy (opj_decompress, libopenjpip), the package is half found and openjpeg.h is not
  # on the include path, so OpenCV builds its bundled OpenJPEG
  if [ "$base" = pi ]; then
    echo -DWITH_AVIF=OFF
    $native || echo -DBUILD_OPENJPEG=ON
  fi
  # ONNX Runtime for cv::dnn ENGINE_ORT (MobileSAM), all targets that have a package
  bo_ort_args
}

configure_opencv() {
  if [ -z "$target" ]; then
    # old interface: configure_opencv [cmake args] with the environment variables
    : ${CC=cc}
    : ${host=$($CC -dumpmachine)}
    download_opencv || return 1
    : ${builddir=$sourcedir/build/$host}
    : ${PROFILE:=full}
    : ${TYPE:=Release}
    : ${arch=$(uname -m)}
    native=true
    base=host
  fi
  bo_pick_cmake || return 1
  prepare_opencv
  fetch_onnxruntime || return 1

  bo_log "cmake $CMAKE, profile $PROFILE, type $TYPE, prefix $prefix, sources $sourcedir"
  args=(
    "${target_args[@]}"
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=${VERBOSE-OFF}
    -DCMAKE_BUILD_TYPE="$TYPE"
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    ${prefix:+-DCMAKE_INSTALL_PREFIX="$prefix"}
    -DBUILD_SHARED_LIBS=ON
    -DENABLE_CCACHE=$(command -v ccache >/dev/null && echo ON || echo OFF)
    ${PKG_CONFIG:+-DPKG_CONFIG_EXECUTABLE="$PKG_CONFIG"}
    ${EXTRA_MODULES:+-DOPENCV_EXTRA_MODULES_PATH="$EXTRA_MODULES"}
    -DOPENCV_GENERATE_PKGCONFIG=ON
    -DOPENCV_ENABLE_NONFREE=ON
  )
  # compilers: only natively, a cross toolchain file sets its own
  if $native; then
    [ -n "$CC" ] && args+=("-DCMAKE_C_COMPILER=$CC")
    [ -n "$CXX" ] && args+=("-DCMAKE_CXX_COMPILER=$CXX")
  fi
  # native toolchain override from the old interface
  [ -z "$target" ] && [ -n "$TOOLCHAIN" ] && args+=("-DCMAKE_TOOLCHAIN_FILE:FILEPATH=$TOOLCHAIN")
  while read -r -a words; do args+=("${words[@]}"); done < <(bo_profile_args)
  # user options last, so they override the profile
  [ -n "$CMAKE_ARGS" ] && { read -r -a words <<<"$CMAKE_ARGS"; args+=("${words[@]}"); }

  if [ -n "$DRYRUN" ]; then
    printf '(cd %s && %s %s' "$builddir" "$CMAKE" "$relsrcdir"
    printf ' %q' "${args[@]}" "$@"
    echo ')'
    return 0
  fi

  # the exit status of a pipeline is tee's: look at cmake's, and at the end of the log
  (cd "$builddir" && "$CMAKE" "$relsrcdir" "${args[@]}" "$@") 2>&1 | tee "$builddir/cmake.log"
  rc=${PIPESTATUS[0]}
  if [ "$rc" != 0 ] || ! grep -q '^-- Configuring done' "$builddir/cmake.log" || grep -q 'Configuring incomplete' "$builddir/cmake.log"; then
    bo_log "configure FAILED (cmake rc=$rc), see $builddir/cmake.log"
    return 1
  fi
  sed -n '/^--   OpenCV modules:/,/^--   Disabled by dependency/p' "$builddir/cmake.log" >"$builddir/to-be-built.txt"
  cat "$builddir/to-be-built.txt"
}

build_opencv() {
  [ -z "$target" ] && { configure_opencv "$@" || return 1; set --; }
  bo_pick_cmake || return 1
  bo_jobs
  tgt=
  for t in $TARGETS; do tgt="$tgt --target $t"; done
  bo_log "building with -j$JOBS${TARGETS:+ ($TARGETS)}"
  "$CMAKE" --build "$builddir" -j"$JOBS" $tgt 2>&1 | tee "$builddir/build.log"
  rc=${PIPESTATUS[0]}
  [ "$rc" = 0 ] || bo_log "build FAILED (rc=$rc), see $builddir/build.log"
  return "$rc"
}

# a populated prefix is never installed over: FORCE=1 to do it anyway
bo_install() {
  bo_pick_cmake || return 1
  dest=$stage_prefix
  if [ -n "$(ls -A "$dest" 2>/dev/null)" ] && [ -z "$FORCE" ]; then
    bo_log "$dest exists and is not empty, not installing over it (FORCE=1 to do it)"
    return 1
  fi
  if $native; then
    "$CMAKE" --install "$builddir" 2>&1 | tail -n 3
  else
    DESTDIR=$STAGE "$CMAKE" --install "$builddir" 2>&1 | tail -n 3
  fi
}

# where the result is and what to give qjs-opencv
bo_where() {
  d=$(ls -d "$stage_prefix"/lib/cmake/opencv* "$stage_prefix"/lib64/cmake/opencv* "$stage_prefix"/sdk/native/jni 2>/dev/null | head -n1)
  echo "target:       $target ($arch${osname}, opencv $ver, profile $PROFILE)"
  echo "build dir:    $builddir"
  echo "prefix:       $prefix  (on the device)"
  $native || echo "staging:      $STAGE   (DESTDIR of the cross install)"
  echo "OPENCV_PREFIX=$stage_prefix"
  echo "OpenCV_DIR=${d:-$stage_prefix/lib/cmake/opencv$major  (not installed yet)}"
}

# copy the staged tree to the device
bo_deploy() {
  $native && { bo_log "deploy is for the cross targets"; return 1; }
  [ -d "$stage_prefix" ] || { bo_log "nothing staged at $stage_prefix (step install)"; return 1; }
  case $base in
  pi)
    : ${PI_HOST:=pi@192.168.178.63}
    : ${PI_DEPLOY:=ci-work/deploy/opencv-$ver-aarch64}   # home relative, the site's work dir
    ssh -o BatchMode=yes -o ConnectTimeout=10 "$PI_HOST" "[ -z \"\$(ls -A $PI_DEPLOY 2>/dev/null)\" ] || [ -n '$FORCE' ] || { echo '$PI_DEPLOY exists, FORCE=1 to overwrite' 1>&2; exit 1; }; mkdir -p $PI_DEPLOY" || return 1
    rsync -a -e 'ssh -o BatchMode=yes -o ConnectTimeout=10' "$stage_prefix/" "$PI_HOST:$PI_DEPLOY/"
    bo_log "deployed to $PI_HOST:~/$PI_DEPLOY (run with LD_LIBRARY_PATH=~/$PI_DEPLOY/lib; move to $prefix with sudo to make it final)"
    ;;
  termux)
    : ${TERMUX_HOST:=termux}
    ssh -o BatchMode=yes -o ConnectTimeout=10 "$TERMUX_HOST" "[ -z \"\$(ls -A $prefix 2>/dev/null)\" ] || [ -n '$FORCE' ] || { echo '$prefix exists, FORCE=1 to overwrite' 1>&2; exit 1; }; mkdir -p $prefix" || return 1
    (cd "$stage_prefix" && tar c .) | ssh -o BatchMode=yes -o ConnectTimeout=10 "$TERMUX_HOST" "tar x -C $prefix"
    bo_log "deployed to $TERMUX_HOST:$prefix"
    ;;
  esac
}

# a tiny program built with the cross compiler, copied to the device and run there
bo_smoke() {
  $native && { bo_log "smoke is for the cross targets"; return 1; }
  [ -d "$stage_prefix/lib" ] || { bo_log "nothing staged at $stage_prefix"; return 1; }
  inc=$(ls -d "$stage_prefix"/include/opencv* | head -n1)
  cat >"$builddir/version.cpp" <<'EOF'
#include <opencv2/core.hpp>
#include <iostream>
int main() { std::cout << "opencv " << cv::getVersionString() << std::endl; return 0; }
EOF
  "$cross_cxx" $cross_flags -std=c++17 -O1 -I"$inc" "$builddir/version.cpp" -o "$builddir/version-$target" \
    -L"$stage_prefix/lib" -lopencv_core -Wl,-rpath-link,"$stage_prefix/lib" || return 1
  file "$builddir/version-$target" | cut -c1-150
  case $base in
  pi)
    : ${PI_HOST:=pi@192.168.178.63}
    : ${PI_DEPLOY:=ci-work/deploy/opencv-$ver-aarch64}
    scp -q -o BatchMode=yes -o ConnectTimeout=10 "$builddir/version-$target" "$PI_HOST:ci-work/version-$target"
    ssh -o BatchMode=yes -o ConnectTimeout=10 "$PI_HOST" "LD_LIBRARY_PATH=\$HOME/$PI_DEPLOY/lib timeout 30 \$HOME/ci-work/version-$target"
    ;;
  termux)
    : ${TERMUX_HOST:=termux}
    scp -q -o BatchMode=yes -o ConnectTimeout=10 "$builddir/version-$target" "$TERMUX_HOST:version-$target"
    ssh -o BatchMode=yes -o ConnectTimeout=10 "$TERMUX_HOST" "LD_LIBRARY_PATH=$prefix/lib:$TERMUX_PREFIX/lib timeout 30 ./version-$target"
    ;;
  esac
}

bo_main() {
  DRYRUN=
  [ "$1" = -n ] && { DRYRUN=1; shift; }
  [ -n "$1" ] || { sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'; return 2; }
  bo_setup_target "$1" || return 1
  shift
  [ $# -gt 0 ] || set -- configure
  for step in "$@"; do
    case $step in
    configure) configure_opencv ;;
    build) build_opencv ;;
    install) bo_install ;;
    deploy) bo_deploy ;;
    smoke) bo_smoke ;;
    where) bo_where ;;
    *) bo_log "unknown step '$step'"; return 2 ;;
    esac || return 1
  done
}

# run only when executed, not when sourced
case ${0##*/} in
build-opencv.sh) bo_main "$@"; exit $? ;;
esac
