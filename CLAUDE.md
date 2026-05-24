# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

OpenCV bindings for QuickJS (https://bellard.org/quickjs/). The build produces a single shared module `opencv.so` (plus a set of per-class `quickjs-*.so` modules) that is loaded from JS as `import * as cv from 'opencv'`. The project is the C++/JS sister to `plot-cv` and is used for live-coding OpenCV pipelines from the qjs REPL.

A parent `CLAUDE.md` at `/mnt/data/Projects/plot-cv/CLAUDE.md` provides behavioral guidelines (think before coding, simplicity first, surgical changes, goal-driven execution). Those apply here too.

## Build

The build is CMake-driven, with helper shell functions in `cfg.sh` that wrap `cmake` for various toolchains and build types. Source `cfg.sh` first to get the `cfg` / `cfg-clang` / `cfg-mingw64` / `cfg-wasm` / `cfg-aarch64` / `cfg-musl*` / `cfg-android*` functions.

The canonical per-host build directory layout is `build/<host-triple>[-<flavor>]`, installed to `inst/<host-triple>[-<flavor>]`. `cmds.sh` shows the three flavors actually in use:

```bash
. ./cfg.sh

# Release (default builddir build/x86_64-linux-gnu)
prefix=/usr/local TYPE=Release cfg \
  -DOpenCV_DIR=/opt/opencv-4.7.0-x86_64/lib/cmake/opencv4

# Debug
CFLAGS="-g3 -ggdb -O0" CXXFLAGS="-g3 -ggdb -O0" \
  prefix=/usr/local TYPE=Debug builddir=build/x86_64-linux-debug cfg \
  -DOpenCV_DIR=/opt/opencv-4.7.0-x86_64/lib/cmake/opencv4

# Profile (gprof)
LDFLAGS="-pg" CFLAGS="-g3 -ggdb -w -pg" CXXFLAGS="-g3 -ggdb -w -pg" \
  prefix=/usr/local TYPE=RelWithDebInfo builddir=build/x86_64-linux-profile cfg \
  -DOpenCV_DIR=/opt/opencv-4.7.0-x86_64/lib/cmake/opencv4

# then
make -C build/x86_64-linux-gnu -j$(nproc)
```

Notes:
- C++ standard is `-std=c++2a`; `DISABLE_WERROR` defaults ON.
- Default `CMAKE_BUILD_TYPE` is `Debug`; pass `TYPE=Release` via `cfg.sh` for an optimized build.
- `build-opencv.sh` (`configure_opencv` / `build_opencv`) is a separate helper that clones and builds OpenCV + opencv_contrib with NONFREE enabled — only needed if there is no system OpenCV.

### Key CMake options (defaults shown)

- `USE_LCCV=ON` — link libcamera via the bundled `LCCV` submodule for Raspberry Pi camera support; pulls in `js_libcamera_app.cpp` and `js_raspi_cam.cpp`.
- `USE_LIBCAMERA=OFF` — alternative path using `libcamera-opencv` submodule.
- `USE_FEATURE2D=ON` — compiles `js_feature2d.cpp` (needs `opencv_xfeatures2d`).
- `USE_BARCODE=ON` — enables barcode detector binding.
- `BUILD_QUICKJS=ON` — also builds QuickJS from `quickjs/` if present, otherwise `find_quickjs()` looks for an installed one (default search expects `/usr/local/include/quickjs/quickjs.h` and `libquickjs.so`).
- `BUILD_SHARED_LIBS=ON`, `QUICKJS_MODULES=ON` — produce a `.so` module under `${QUICKJS_C_MODULE_DIR}` (default `/usr/local/lib/<arch>/quickjs`).

### Submodules

`pngpp`, `giflib-turbo`, `gifenc`, `libcamera-opencv`, `LCCV` (see `.gitmodules`). Run `git submodule update --init --recursive` after clone. `LCCV` is built in-tree when `USE_LCCV=ON`.

## Running JS code

After build & install, qjs picks up `opencv.so` via `QUICKJS_MODULES` env var or the compile-time module dirs:

```bash
QUICKJS_MODULES=$PWD/build/x86_64-linux-gnu qjs tests/test_freetype.js
# or after `make install`
qjs tests/test_video_writer.js
```

Tests under `tests/*.js` are standalone scripts (no test runner) — run them individually with `qjs`. Several rely on companion assets in `tests/` (e.g. `box.png`, `box_in_scene.png`, `model.yml.gz`).

The high-level JS wrappers under `js/` (`cvHighGUI.js`, `cvPipeline.js`, `cvVideo.js`, `cvUtils.js`) provide `Window`, `TextStyle`, `Pipeline`, `VideoSource`, `ImageSequence` abstractions built on top of the C++ bindings — they're imported as `'../js/cvHighGUI.js'` etc. from the tests.

## Architecture

### One file per OpenCV concept

Each `js_<thing>.cpp` is a self-contained binding for one OpenCV class or module:

- **Value types** with their own `JSClassID`: `js_mat.cpp`, `js_umat.cpp`, `js_contour.cpp`, `js_point.cpp`, `js_rect.cpp`, `js_size.cpp`, `js_rotated_rect.cpp`, `js_line.cpp`, `js_keypoint.cpp`, `js_matx.cpp`, `js_affine3.cpp`.
- **Iterators**: `js_mat_iterator` (inside `js_mat.cpp`), `js_point_iterator.cpp`, `js_line_iterator.cpp`, `js_slice_iterator.cpp`.
- **Modules** of free functions: `js_cv.cpp` (the catch-all; ~85 KB), `js_imgproc.cpp`, `js_draw.cpp`, `js_highgui.cpp`, `js_calib3d.cpp`, `js_dnn.cpp`, `js_ximgproc.cpp`, `js_algorithms.cpp`, `js_utility.cpp`.
- **Stateful detectors/decoders**: `js_clahe.cpp`, `js_subdiv2d.cpp`, `js_feature2d.cpp`, `js_fast_line_detector.cpp`, `js_line_segment_detector.cpp`, `js_barcode_detector.cpp`, `js_bg_subtractor.cpp`, `js_aruco.cpp`, `js_fisheye.cpp`, `js_white_balancer.cpp`, `js_filenode.cpp`, `js_filestorage.cpp`, `js_commandlineparser.cpp`.
- **IO**: `js_video_capture.cpp`, `js_video_writer.cpp`, `js_raspi_cam.cpp`, `js_libcamera_app.cpp`, `js_opengl.cpp`.

`src/init_module.cpp` is the entry point — `JS_INIT_MODULE` calls per-file `js_*_init` functions to register classes and exports.

### `cmake/JSBindings.cmake` is the wiring

`make_shared_module(opencv ${OPENCV_SOURCES})` (in `CMakeLists.txt`) compiles everything into one `opencv.so`. The per-class shared modules (`quickjs-mat`, `quickjs-point`, etc.) are only built when `make_js_bindings()` is invoked — in that branch the explicit `target_link_libraries(quickjs-cv quickjs-mat ...)` chain in `JSBindings.cmake` defines the inter-module dependency graph. Read that block before splitting or adding a new `js_<x>.cpp` so the inter-module reference graph stays acyclic.

### `include/jsbindings.hpp` is the helper layer

Common JS↔C++ glue lives in `include/jsbindings.hpp` (single file, ~1000 lines): templated `js_value_to` / `js_value_from`, `js_number_*`, `js_color_*`, `js_scalar_*`, `js_arraybuffer_*`, iterator helpers, `JSConstructor` for registering classes. The other key headers: `js_array.hpp`, `js_typed_array.hpp`, `js_alloc.hpp`, `util.hpp`, `geometry.hpp`. Reuse these helpers — they handle the JSValue lifetime correctly.

### Memory model

The README's design intent: **no copies, mutable, finalizers do the work.** A `cv.Mat` is backed by a `cv::Mat`; iteration yields `Float64Array(4)` views into the underlying buffer; `cv.Contour` is a `std::vector<cv::Point3d>` exposed as an iterable ArrayBuffer. Many functions accept `cv.Mat | cv.Contour | TypedArray` interchangeably because they unwrap through `cv::_InputArray` / `cv::_InputOutputArray` (`JSInputArgument`, `JSImageArgument` in `jsbindings.hpp`). When editing a binding, preserve this: do not silently copy through `cv::Mat::clone()` or allocate a new buffer just to simplify the signature.

### Algorithms

`algorithms/` contains in-tree implementations not in OpenCV but interoperable on `cv::Mat`: `skeletonization.hpp`, `trace_skeleton.hpp`, `pixel_neighborhood.hpp`, `palette.hpp`, `dominant_colors_grabber.cpp`. `lsd/` contains a Line Segment Detector. `gifenc/` and `giflib-turbo/` provide GIF encoding (low-bit-depth palette work referenced in the README).

## Conventions to keep

- Match the existing style — `.clang-format` and `.cmake-format` are checked in; don't reflow files you didn't touch.
- New bindings go in their own `js_<name>.cpp` + matching `.hpp`. `CMakeLists.txt`'s `file(GLOB OPENCV_SOURCES js_*.[ch]pp ...)` picks them up automatically, but if the module depends on another one (e.g. mat → size), add the `target_link_libraries(...)` line in `cmake/JSBindings.cmake`.
- The `js_<name>.cpp` skeleton: one `JSClassID`, one `JSClassDef` with a finalizer, `js_<name>_proto_funcs` table, `js_<name>_static_funcs`, and a `js_<name>_init(ctx, m)` function called from `init_module.cpp`.
- Prefer the `js_value_to` / `js_value_from` templates over hand-rolled `JS_To*` calls — they already cover `cv::Vec`, `cv::Scalar_`, `std::vector<T>`, `cv::Range`.
