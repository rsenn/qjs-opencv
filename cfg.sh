cfg() {
  ({
    if type gcc 2>/dev/null >/dev/null && type g++ 2>/dev/null >/dev/null; then
      : ${CC:=gcc} ${CXX:=g++}
    elif type clang 2>/dev/null >/dev/null && type clang++ 2>/dev/null >/dev/null; then
      : ${CC:=clang} ${CXX:=clang++}
    fi

    : ${build:=$($CC -dumpmachine | sed 's|-pc-|-|g')}

    if [ -z "$host" -a -z "$builddir" ]; then
      host=$build
      case "$host" in
      x86_64-w64-mingw32)
        host="$host"
        : ${builddir=build/$host}
        ;;
      i686-w64-mingw32)
        host="$host"
        : ${builddir=build/$host}
        ;;
      x86_64-pc-*)
        host="$host"
        : ${builddir=build/$host}
        ;;
      i686-pc-*)
        host="$host"
        : ${builddir=build/$host}
        ;;
      esac
    fi

    if false && [ -n "$host" -a -z "$prefix" ]; then
      case "$host" in
      x86_64-w64-mingw32) : ${prefix=/mingw64} ;;
      i686-w64-mingw32) : ${prefix=/mingw32} ;;
      x86_64-pc-*) : ${prefix=/usr} ;;
      i686-pc-*) : ${prefix=/usr} ;;
      esac
    fi

    : ${prefix:=/usr/local}
    : ${libdir:=$prefix/lib}
    [ -d "$libdir/$host" ] && libdir=$libdir/$host

    if [ -e "$TOOLCHAIN" ]; then
      cmakebuild=$(basename "$TOOLCHAIN" .cmake)
      cmakebuild=${cmakebuild%.toolchain}
      cmakebuild=${cmakebuild#toolchain-}
      : ${builddir=build/$cmakebuild}
    else
      : ${builddir=build/$host}
    fi
    case "$host" in
    *msys*) ;;
    *) test -n "$builddir" && builddir=$(echo $builddir | sed 's|-pc-|-|g') ;;
    esac

    case $(uname -o) in
    # MSys|MSYS|Msys) SYSTEM="MSYS" ;;
    *) SYSTEM="Unix" ;;
    esac

    case "$STATIC:$TYPE" in
    YES:* | yes:* | y:* | 1:* | ON:* | on:* | *:*[Ss]tatic*) set -- "$@" \
      -DBUILD_SHARED_LIBS=OFF \
      -DENABLE_PIC=OFF ;;
    esac

    [ -n "$PKG_CONFIG_PATH" ] && echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH" 1>&2
    [ -n "$PKG_CONFIG" ] && case "$PKG_CONFIG" in
    */*) ;;
    *) PKG_CONFIG=$(which "$PKG_CONFIG") ;;
    esac
    : ${generator:="CodeLite - Unix Makefiles"}

    mkdir -p $builddir
    : ${relsrcdir=$(realpath --relative-to "$builddir" .)}
    echo "Entering directory '$builddir'" 1>&2
    : set -x
    cd "${builddir:-.}"
    IFS="$IFS "
    set -- -Wno-dev \
      -G "$generator" \
      ${prefix:+-DCMAKE_INSTALL_PREFIX="$prefix"} \
      ${VERBOSE:+-DCMAKE_VERBOSE_MAKEFILE=${VERBOSE:-OFF}} \
      -DCMAKE_BUILD_TYPE="${TYPE:-Debug}" \
      -DBUILD_SHARED_LIBS=ON \
      ${CC:+-DCMAKE_C_COMPILER="$CC"} \
      ${CXX:+-DCMAKE_CXX_COMPILER="$CXX"} \
      ${PKG_CONFIG:+-DPKG_CONFIG_EXECUTABLE="$PKG_CONFIG"} \
      ${TOOLCHAIN:+-DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"} \
      ${CC:+-DCMAKE_C_COMPILER="$CC"} \
      ${CXX:+-DCMAKE_CXX_COMPILER="$CXX"} \
      ${MAKE:+-DCMAKE_MAKE_PROGRAM="$MAKE"} \
      "$@" \
      $relsrcdir
    eval "${CMAKE:-cmake} \"\$@\""
  } 2>&1 && echo "Configured in '$builddir'" 1>&2) | tee "${builddir##*/}.log"
}

cfg-android() {
  (
    : ${builddir=build/android}
    cfg \
      -DCMAKE_INSTALL_PREFIX=/opt/arm-linux-androideabi/sysroot/usr \
      \
      -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN:-/opt/android-cmake/android.cmake} \
      -DANDROID_NATIVE_API_LEVEL=21 \
      -DPKG_CONFIG_EXECUTABLE=arm-linux-androideabi-pkg-config \
      -DCMAKE_PREFIX_PATH=/opt/arm-linux-androideabi/sysroot/usr \
      -DCMAKE_MAKE_PROGRAM=/usr/bin/make \
      -DCMAKE_MODULE_PATH="/opt/OpenCV-3.4.1-android-sdk/sdk/native/jni/abi-armeabi-v7a" \
      -DOpenCV_DIR="/opt/OpenCV-3.4.1-android-sdk/sdk/native/jni/abi-armeabi-v7a" \
      "$@"
  )
}

cfg-diet() {
  (
    : ${build=$(gcc -dumpmachine | sed 's|-pc-|-|g')}
    : ${host=${build/-gnu/-diet}}
    : ${prefix=/opt/diet}
    : ${libdir=/opt/diet/lib-${host%%-*}}
    : ${bindir=/opt/diet/bin-${host%%-*}}

    CC="diet-gcc"

    export CC

    if type pkgcfg >/dev/null; then
      export PKG_CONFIG=$(type pkgcfg 2>&1 | sed 's,.* is ,,')
    elif type pkg-config >/dev/null; then
      export PKG_CONFIG=$(type pkg-config 2>&1 | sed 's,.* is ,,')
    fi

    : ${builddir=build/${host%-*}-diet}
    prefix=/opt/diet

    export builddir prefix
    PKG_CONFIG_PATH=/opt/diet/lib-x86_64/pkgconfig:/opt/diet/lib/pkgconfig:/usr/lib/diet/lib/pkgconfig \
      cfg \
      -DCMAKE_INSTALL_PREFIX="$prefix" \
      -DBUILD_SSL=OFF \
      -DBUILD_SHARED_LIBS=OFF \
      -DENABLE_SHARED=OFF \
      -DSHARED_LIBS=OFF \
      -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_FIND_ROOT_PATH="$prefix" \
      -DCMAKE_SYSTEM_LIBRARY_PATH="$prefix/lib-${host%%-*}" \
      ${launcher:+-DCMAKE_C_COMPILER_LAUNCHER="$launcher"} \
      -DPKG_CONFIG_EXECUTABLE="$PKG_CONFIG" \
      "$@"
  )
}

cfg-diet64() {
  (
    build=$(gcc -dumpmachine | sed 's|-pc-|-|g')
    host=${build%%-*}-linux-diet
    host=x86_64-${host#*-}

    export prefix=/opt/diet

    builddir=build/$host \
      PKG_CONFIG_PATH=/opt/diet/lib-x86_64/pkgconfig:/usr/lib/diet/lib-x86_64/pkgconfig \
      CC="diet-gcc" \
      cfg-diet \
      -DCMAKE_SYSTEM_LIBRARY_PATH=/opt/diet/lib-x86_64 \
      "$@"
  )
}

cfg-diet32() {
  (
    build=$(gcc -dumpmachine | sed 's|-pc-|-|g')
    host=${build%%-*}-linux-diet
    host=i686-${host#*-}

    if type diet32-clang 2>/dev/null >/dev/null; then
      CC="diet32-clang"
      export CC
    elif type diet32-gcc 2>/dev/null >/dev/null; then
      CC="diet32-gcc"
      export CC
    else
      CC="gcc"
      launcher="/opt/diet/bin-i386/diet"
      CFLAGS="-m32"
      export CC launcher CFLAGS
    fi

    builddir=build/$host \
      PKG_CONFIG_PATH=/opt/diet/lib-i386/pkgconfig:/usr/lib/diet/lib-i386/pkgconfig \
      cfg-diet \
      -DCMAKE_SYSTEM_LIBRARY_PATH=/opt/diet/lib-i386 \
      "$@"
  )
}

cfg-mingw() {
  (
    build=$(gcc -dumpmachine)
    : ${host=${build%%-*}-w64-mingw32}
    : ${prefix=/usr/$host/sys-root/mingw}

    case "$host" in
    x86_64-*) : ${TOOLCHAIN=/opt/cmake-toolchains/mingw64.cmake} ;;
    *) : ${TOOLCHAIN=/opt/cmake-toolchains/mingw32.cmake} ;;
    esac

    : ${PKG_CONFIG_PATH=/usr/${host}/sys-root/mingw/lib/pkgconfig}

    export TOOLCHAIN PKG_CONFIG_PATH

    builddir=build/$host \
      bindir=$prefix/bin \
      libdir=$prefix/lib \
      cfg \
      "$@"
  )
}
cfg-mingw32() {
  host=i686-w64-mingw32 cfg-mingw "$@"
}
cfg-mingw64() {
  host=x86_64-w64-mingw32 cfg-mingw "$@"
}

cfg-emscripten() {
  (
    build=$(gcc -dumpmachine | sed 's|-pc-|-|g')
    host=${build/-gnu/-emscriptenlibc}
    builddir=build/${host%-*}-emscripten

    prefix=$(which emcc | sed 's|/emcc$|/system|')
    libdir=$prefix/lib
    bindir=$prefix/bin

    CC="emcc" \
      PKG_CONFIG="PKG_CONFIG_PATH=$libdir/pkgconfig pkg-config" \
      cfg \
      -DCMAKE_INSTALL_PREFIX="$prefix" \
      -DENABLE_SHARED=OFF \
      -DSHARED_LIBS=OFF \
      -DBUILD_SHARED_LIBS=OFF \
      "$@"
  )
}

cfg-clang() {
  (
    build=$(cc -dumpmachine | sed 's|-pc-|-|g')
    build=${build/-gnu/-clang}
    : ${host=$build}
    : ${builddir=build/$host}
    : ${prefix=/usr/local}

    CC=clang CXX=clang++ \
      cfg \
      "$@"
  )
}

cfg-tcc() {
  (
    build=$(cc -dumpmachine | sed 's|-pc-|-|g')
    host=${build/-gnu/-tcc}
    builddir=build/$host
    prefix=/usr/local
    includedir=/usr/lib/$build/tcc/include
    libdir=/usr/lib/$build/tcc/
    bindir=/usr/bin

    CC=${TCC:-tcc} \
      cfg \
      "$@"
  )
}

cfg-musl() {
  (
    : ${build=$(gcc -dumpmachine | sed 's|-pc-|-|g')}
    : ${host=${build%-*}-musl}

    : ${prefix=/opt/musl}
    : ${includedir=$prefix/include/$host}
    : ${libdir=$prefix/lib/$host}
    : ${bindir=$prefix/bin/$host}
    : ${builddir=build/$host}

    CC=/usr/bin/musl-gcc \
      PKG_CONFIG=musl-pkg-config \
      PKG_CONFIG_PATH=/opt/musl/lib/pkgconfig:/usr/lib/${host%%-*}-linux-musl/pkgconfig \
      cfg \
      -DENABLE_SHARED=OFF \
      -DSHARED_LIBS=OFF \
      -DBUILD_SHARED_LIBS=OFF \
      "$@"
  )
}

cfg-musl64() {
  (
    build=$(gcc -dumpmachine | sed 's|-pc-|-|g')
    host=${build%%-*}-linux-musl
    host=x86_64-${host#*-}

    : ${builddir=build/$host}

    CFLAGS="-m64" \
      cfg-musl \
      -DCMAKE_C_COMPILER="musl-gcc" \
      "$@"
  )
}

cfg-musl32() {
  (
    build=$(gcc -dumpmachine | sed 's|-pc-|-|g')
    host=$(echo "$build" | sed "s|x86_64|i686| ; s|-gnu|-musl|")

    : ${builddir=build/$host}

    CFLAGS="-m32" \
      cfg-musl \
      -DCMAKE_C_COMPILER="musl-gcc" \
      "$@"
  )
}

cfg-msys() {
  (
    echo "host: $host"
    build=$(gcc -dumpmachine)
    : ${host=${build%%-*}-pc-msys}
    : ${prefix=/usr/$host/sysroot/usr}
    echo "host: $host"
    : ${PKG_CONFIG_PATH=/usr/${host}/sysroot/usr/lib/pkgconfig}

    export PKG_CONFIG_PATH

    case "$host" in
    x86_64*) TOOLCHAIN=/opt/cmake-toolchains/msys64.cmake ;;
    *) TOOLCHAIN=/opt/cmake-toolchains/msys32.cmake ;;
    esac
    export TOOLCHAIN
    echo "builddir: $builddir"

    : ${builddir=build/$host}

    bindir=$prefix/bin \
      libdir=$prefix/lib \
      host=$host \
      build=$build \
      cfg \
      "$@"
  )
}

cfg-msys32() {
  host=i686-pc-msys \
    cfg-msys "$@"
}

cfg-msys64() {
  host=x86_64-pc-msys \
    cfg-msys "$@"
}

cfg-termux() {
  (
    : ${builddir=build/termux}
    cfg \
      -DCMAKE_INSTALL_PREFIX=/data/data/com.termux/files/usr \
      \
      -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN:-/opt/android-cmake/android.cmake} \
      -DANDROID_NATIVE_API_LEVEL=21 \
      -DPKG_CONFIG_EXECUTABLE=arm-linux-androideabi-pkg-config \
      -DCMAKE_PREFIX_PATH=/data/data/com.termux/files/usr \
      -DCMAKE_MAKE_PROGRAM=/usr/bin/make \
      -DCMAKE_MODULE_PATH="/data/data/com.termux/files/usr/lib/cmake" \
      "$@"
  )
}
cfg-wasm() {
  export VERBOSE
  (
    EMCC=$(which emcc)
    EMSCRIPTEN=$(dirname "$EMCC")
    EMSCRIPTEN=${EMSCRIPTEN%%/bin*}
    test -f /opt/cmake-toolchains/generic/Emscripten-wasm.cmake && TOOLCHAIN=/opt/cmake-toolchains/generic/Emscripten-wasm.cmake
    test '!' -f "$TOOLCHAIN" && TOOLCHAIN=$(find "$EMSCRIPTEN" -iname emscripten.cmake)
    test -f "$TOOLCHAIN" || unset TOOLCHAIN
    : ${prefix:="$EMSCRIPTEN"}
    : ${builddir=build/emscripten-wasm}

    CC="$EMCC" \
      cfg \
      -DEMSCRIPTEN_PREFIX="$EMSCRIPTEN" \
      ${TOOLCHAIN:+-DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"} \
      -DCMAKE_EXE_LINKER_FLAGS="-s WASM=1" \
      -DCMAKE_EXECUTABLE_SUFFIX=".html" \
      -DCMAKE_EXECUTABLE_SUFFIX_INIT=".html" \
      -DUSE_{ZLIB,BZIP,LZMA,SSL}=OFF \
      "$@"
  )
}

cfg-tcc() {
  (
    build=$(cc -dumpmachine | sed 's|-pc-|-|g')
    host=${build/-gnu/-tcc}
    : ${builddir=build/$host}
    prefix=/usr/local
    includedir=/usr/lib/$build/tcc/include
    libdir=/usr/lib/$build/tcc/
    bindir=/usr/bin

    CC=${TCC:-tcc} \
      cfg \
      "$@"
  )
}

cfg-android64() {
  (
    : ${builddir=build/android64}
    cfg -DCMAKE_INSTALL_PREFIX=/opt/aarch64-linux-android64eabi/sysroot/usr -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN:-/opt/android64-cmake/android64.cmake} -DANDROID_NATIVE_API_LEVEL=21 -DPKG_CONFIG_EXECUTABLE=aarch64-linux-android64eabi-pkg-config -DCMAKE_PREFIX_PATH=/opt/aarch64-linux-android64eabi/sysroot/usr -DCMAKE_MAKE_PROGRAM=/usr/bin/make -DCMAKE_MODULE_PATH="/opt/OpenCV-3.4.1-android64-sdk/sdk/native/jni/abi-armeabi-v7a" -DOpenCV_DIR="/opt/OpenCV-3.4.1-android64-sdk/sdk/native/jni/abi-armeabi-v7a" "$@"
  )
}

cfg-emscripten() {
  (
    build=$(cc -dumpmachine | sed 's|-pc-|-|g')
    host=$(emcc -dumpmachine)
    : ${builddir=build/${host%-*}-emscripten}
    : ${prefix=$EMSCRIPTEN/system}
    : ${libdir=$prefix/lib}
    : ${bindir=$prefix/bin}
    : ${EMSCRIPTEN=$EMSDK/upstream/emscripten}
    export TOOLCHAIN="${EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake"

    PREFIX_PATH=$(
      set -- /opt/*-wasm
      IFS=";"
      echo "$*"
    )
    LIBRARY_PATH=$(
      set -- /opt/*-wasm/lib
      IFS=";"
      echo "$*"
    )
    PKG_CONFIG_PATH=$(
      set -- /opt/*-wasm/lib/pkgconfig
      IFS=":"
      echo "$*"
    ) #${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}
    PKG_CONFIG_PATH="${PKG_CONFIG_PATH:+$PKG_CONFIG_PATH:}${EMSCRIPTEN}/system/lib/pkgconfig"
    export PKG_CONFIG_PATH
    echo PKG_CONFIG_PATH="${PKG_CONFIG_PATH}"
    CC="emcc" CXX="em++" TYPE="Release" \
      CFLAGS="'-sWASM=1 -sUSE_PTHREADS=0 -sLLD_REPORT_UNDEFINED'" \
      CXXFLAGS="'-sWASM=1 -sUSE_PTHREADS=0 -sLLD_REPORT_UNDEFINED'" \
      CMAKE_WRAPPER="emcmake" \
      prefix=/opt/${PWD##*/}-wasm \
      cfg \
      -DCMAKE_PREFIX_PATH="$PREFIX_PATH" \
      -DCMAKE_LIBRARY_PATH="$LIBRARY_PATH" \
      -DENABLE_PIC=FALSE \
      "$@"
  )
}

cfg-aarch64() {
  (
    : ${build=$(cc -dumpmachine | sed 's|-pc-|-|g')}
    : ${host=aarch64-${build#*-}}
    : ${builddir=build/$host}

    : ${prefix=/usr/aarch64-linux-gnu/sysroot/usr}

    : ${TOOLCHAIN=/opt/cmake-toolchains/aarch64-linux-gnu.toolchain.cmake}
    export prefix TOOLCHAIN

    PKG_CONFIG=$(which ${host}-pkg-config) \
      cfg "$@"
  )
}
