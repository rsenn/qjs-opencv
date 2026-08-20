# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

OpenCV bindings for QuickJS (https://bellard.org/quickjs/). The build produces a single shared module `opencv.so` (plus a set of per-class `quickjs-*.so` modules) that is loaded from JS as `import * as cv from 'opencv'`. The project is the C++/JS sister to `plot-cv` and is used for live-coding OpenCV pipelines from the qjs REPL.

A parent `CLAUDE.md` at `/mnt/data/Projects/plot-cv/CLAUDE.md` provides behavioral guidelines (think before coding, simplicity first, surgical changes, goal-driven execution). Those apply here too.

The roadmap is tracked in `TODO.md`. Bugs are tracked in a plain text file `BUGS` (all lowercase), formatted like `../../../shish/BUGS` and `../quickjs/qjs-nanovg/BUGS`: each entry starts with `- <canonical-name>: <description>`, and if there's a JS repro it follows after a blank line, indented. This `BUGS` file is to be updated automatically as soon as a bug is found during work in this repo — don't wait to be asked:

```
- <canonical-name>: <description>

    <JS code that triggers it>

- <next-canonical-name>: <description>
```

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

After build, use `qjsm` (not `qjs`) with `QUICKJS_MODULE_PATH` pointing to the build directory:

```bash
QUICKJS_MODULE_PATH=$PWD/build/x86_64-linux-gnu qjsm tests/unittests/test_contour_functions.js
```

Or after `make install`, `qjsm` picks up `opencv.so` from the system module dirs.

Tests under `tests/*.js` are standalone scripts. Unit tests using `tinytest` are in `tests/unittests/`. Several tests rely on companion assets in `tests/` (e.g. `box.png`, `box_in_scene.png`, `model.yml.gz`).

The high-level JS wrappers under `js/` (`cvHighGUI.js`, `cvPipeline.js`, `cvVideo.js`, `cvUtils.js`) provide `Window`, `TextStyle`, `Pipeline`, `VideoSource`, `ImageSequence` abstractions built on top of the C++ bindings.

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

### Contour Migration Strategy (Phase 1 COMPLETE)

**Status:** Phase 1 complete (2026-08-13). Phase 2 (MatVector) is lower priority.

The project is migrating from the custom `Contour` class toward opencv.js-compatible APIs. **Phase 1 (method migration) is done**; Phase 2 (MatVector class) is planned but secondary.

**Phase 1 — COMPLETE:**
All Contour methods are now available as freestanding functions:
- 16 shape analysis functions (already existed): `cv.contourArea()`, `cv.arcLength()`, `cv.boundingRect()`, `cv.approxPolyDP()`, `cv.convexHull()`, `cv.fitEllipse()`, `cv.fitLine()`, `cv.isContourConvex()`, `cv.minAreaRect()`, `cv.minEnclosingCircle()`, `cv.minEnclosingTriangle()`, `cv.pointPolygonTest()`, `cv.rotatedRectangleIntersection()`, `cv.convexityDefects()`, `cv.matchShapes()`, `cv.HuMoments()`
- 7 psimpl methods migrated to `cv.psimpl.*` namespace: `cv.psimpl.douglasPeucker()`, `cv.psimpl.nthPoint()`, `cv.psimpl.radialDistance()`, `cv.psimpl.reumannWitkam()`, `cv.psimpl.opheim()`, `cv.psimpl.lang()`, `cv.psimpl.perpendicularDistance()`
- All accept Mat CV_32SC2 (findContours output format), Contour objects (backward compat), or JS arrays
- All return Mat CV_32SC2

**Phase 2 — MatVector (planned, secondary):**
Implement `MatVector` class wrapping `std::vector<cv::Mat>`. See BUGS entry `no-opencvjs-matvector` for detailed plan.

**Phase 3 — Cleanup (ongoing):**
Remove non-standard Contour methods, assess which functions need `Contour<T>` internally vs. generic `JSInputOutputArray`.

**Key insight (binary compatibility):** Point and Vec2i have identical memory layout (8 bytes). Mat CV_32SC2 data can be reinterpreted as Vec2i* for zero-copy iteration. This enables efficient iteration without requiring full MatVector migration. See BUGS: `make-dormant-point-line-iterator-plug-into-point-mat-vector`.

### Testing

Tests are in `tests/unittests/` using the `tinytest` framework (copied from `../quickjs/qjs-modules/tests/tinytest.js`). Each functionality group gets its own test file:
- `test_contour_functions.js` — 16 shape analysis tests
- `test_psimpl_functions.js` — 9 psimpl simplification tests
- `test_matvector.js` — planned for MatVector

**Run tests with `qjsm` (not `qjs`!)** and set `QUICKJS_MODULE_PATH` to the build directory:
```bash
QUICKJS_MODULE_PATH=build/x86_64-linux-gnu qjsm tests/unittests/test_contour_functions.js
```

The `qjsm` binary supports ES modules natively. Do NOT use `qjs` or `-m`/`--module` flags.

## Conventions to keep

- **Use `qjsm` for running scripts, not `qjs`.** The `qjsm` binary has native ES module support. Do NOT use `-m` or `--module` flags (those load library modules, not scripts).
- **Use `QUICKJS_MODULE_PATH=<build-dir>`** when testing with freshly built `opencv.so` to ensure the local build is loaded instead of an installed version.
- **Use `JS_CFUNC_MAGIC_DEF` for method groups.** When adding a set of related methods to a binding file, always use magic dispatch: one handler function with a switch on the magic value, registered via `JS_CFUNC_MAGIC_DEF`. Don't create separate C++ functions per method. This matches existing patterns like `js_imgproc_shape()`, `js_mat_expr()`, `js_contour_psimpl()`.
- **Namespace non-standard extensions.** Custom utilities like psimpl go under a namespace object (e.g., `cv.psimpl.douglasPeucker`), mirroring how `cv.dnn.*` is done in `js_dnn.cpp`. Use `JS_OBJECT_DEF` with the function list to create the namespace in the exports array.
- Match the existing style — `.clang-format` and `.cmake-format` are checked in; don't reflow files you didn't touch.
- New bindings go in their own `js_<name>.cpp` + matching `.hpp`. `CMakeLists.txt`'s `file(GLOB OPENCV_SOURCES js_*.[ch]pp ...)` picks them up automatically, but if the module depends on another one (e.g. mat → size), add the `target_link_libraries(...)` line in `cmake/JSBindings.cmake`.
- The `js_<name>.cpp` skeleton: one `JSClassID`, one `JSClassDef` with a finalizer, `js_<name>_proto_funcs` table, `js_<name>_static_funcs`, and a `js_<name>_init(ctx, m)` function called from `init_module.cpp`.
- Prefer the `js_value_to` / `js_value_from` templates over hand-rolled `JS_To*` calls — they already cover `cv::Vec`, `cv::Scalar_`, `std::vector<T>`, `cv::Range`.
- **Mat typed data views are now implemented.** Mat has `.data`, `.data8S`, `.data16U`, `.data16S`, `.data32S`, `.data32F`, `.data64F` properties returning typed array views of the pixel buffer (via `js_typedarray<T>::from_buffer()` template). The legacy `.array` property is disabled but `.buffer` (raw ArrayBuffer) remains.
- **Scalar.all(v) is implemented.** Creates a Float64Array with all 4 components set to v, matching opencv.js API. Implemented via `JS_NewCFunctionMagic` since it's a static method on the Scalar constructor.

## Key files for reference

- `BUGS` — known issues and opencv.js API discrepancies (plain text, see format above)
- `TODO.md` — binding backlog and migration roadmap
- `js_mat.cpp` — Mat class (most complex binding, ~2000 lines, reference for patterns)
- `js_psimpl.cpp` — `cv.psimpl.*` namespace (polyline simplification)
- `js_imgproc.cpp` — shape analysis functions (SHAPE_* magic enum pattern)
- `js_dnn.cpp` — DNN namespace pattern (reference for namespace objects)
- `include/js_typed_array.hpp` — `js_typedarray<T>::from_buffer()` template
- `include/jsbindings.hpp` — common JS↔C++ glue (~1000 lines)
- `test_matvector.cpp` / `test_pointvector_contours.cpp` — C++ feasibility tests for MatVector
- `test_binary_compat.cpp` — binary compatibility verification (Point=Vec2i, Vec4i=2×Point)
- `doc/opencv-js-api.md` — target API specification (what opencv.js exposes)
