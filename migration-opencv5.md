# OpenCV 5.0 migration

## Executive summary

Building `qjs-opencv` against `/opt/opencv-5.0.0-x86_64` originally produced **144 compile
errors across 13 files**. At a glance this looked like a severe loss of API surface. It wasn't:
investigating each error against the actual OpenCV 5.0.0 headers showed that **the large
majority of "missing" functions still exist in OpenCV 5** — they were relocated to new
headers/namespaces/modules, or had a call-site detail change (a method became a field, an
implicit conversion was dropped, an enum stopped accepting a bare `int`). Only a small, specific
set of things were genuinely removed.

All 144 errors are now fixed. `qjs-opencv` builds cleanly against OpenCV 4.11.0, 4.13.0, and
5.0.0 from the same source tree, gated by CMake capability probes and `#ifdef`s — no source-level
fork.

| # | Root cause | Files (error count) | Real removal? | Fix type |
|---|---|---|---|---|
| 1 | Shape/contour functions moved `imgproc` → new `geometry` module | `js_imgproc.cpp` (34), `js_contour.cpp` (26), `js_subdiv2d.cpp` (26), `src/geometry.cpp` (1) | Only `linearPolar`/`logPolar` inside this group | Guarded `#include`, mostly |
| 2 | `MatSize::dims()` method → `MatShape::dims` field | `js_mat.cpp` (7), `js_umat.cpp` (1), `dominant_colors_grabber.cpp` (1) | No | Portable, no guard |
| 3 | Chessboard-calibration functions moved `calib3d` → `objdetect` | `js_calib3d.cpp` (16) | No | Portable, no guard |
| 4 | `AKAZE`/`BRISK`/`KAZE`/`AgastFeatureDetector` moved `features2d` → `xfeatures2d` | `js_feature2d.cpp` (4) | No (needs contrib) | Guarded namespace |
| 5 | DNN engine rewrite (importers dropped, `Net` methods dropped, `DataLayout` moved) | `js_dnn.cpp` (24) | Partially (see below) | Guarded, one flag |
| 6 | `BarcodeDetector` ctor signature changed | `js_barcode_detector.cpp` (1) | Partially | Guarded |
| 7 | `LineSegmentDetectorModes` stopped accepting implicit `int` | `js_line_segment_detector.cpp` (1) | No | Portable, no guard |
| 8 | `opencv2/core/core_c.h` (C API) removed entirely | `js_cv.cpp` (1 fatal, masked ~9 more) | No (header unneeded) | Portable, no guard |
| 9 | Deprecated `VideoCapture` backend constants + `convertFp16` removed | `js_cv.cpp` (10 after #8 unmasked them) | Yes | Guarded |

Two more, unrelated pre-existing bugs were found and fixed along the way (both blocked a clean
configure once the real migration work started exercising code paths that had been dormant) — see
"Bugs found and fixed" below and `BUGS`.

## Root-cause catalog

### 1. Shape/contour-analysis functions: new `geometry` module

OpenCV 5.0 introduced `opencv2/geometry.hpp` (and `opencv2/geometry/2d.hpp`), backed by a new
`libopencv_geometry.so`, and moved a large batch of functions out of `imgproc` into it:
`boundingRect`, `moments`, `HuMoments`, `matchShapes`, `approxPolyDP`, `arcLength`,
`contourArea`, `convexHull`, `convexityDefects`, `isContourConvex`, `intersectConvexConvex`,
`fitEllipse`/`fitEllipseAMS`/`fitEllipseDirect`, `fitLine`, `minAreaRect`, `boxPoints`,
`minEnclosingCircle`/`minEnclosingTriangle`, `pointPolygonTest`, `rotatedRectangleIntersection`,
`getAffineTransform`, `getPerspectiveTransform`, `getRotationMatrix2D`,
`invertAffineTransform`, and `cv::Subdiv2D`. All of them are still fully present and functional —
just declared in a different header. `js_subdiv2d.cpp`'s 26 errors were all a single cascading
failure from `cv::Subdiv2D` being undeclared (the constructor's `cv::Subdiv2D* s;` local failed to
declare, so every later use of `s` failed too).

**Fix**: a new CMake probe, `HAVE_OPENCV2_GEOMETRY_HPP` (`check_include_file_cxx`), gates a
guarded `#include <opencv2/geometry.hpp>` added to `js_imgproc.cpp`, `js_contour.cpp`,
`js_subdiv2d.cpp`, `js_mat.cpp`, and `src/geometry.cpp`. No other changes needed —
`opencv_geometry` is already linked automatically (all `OpenCV_LIBS` from `find_package(OpenCV
CONFIG)` get linked, confirmed via `link.txt`).

**Related**: `goodFeaturesToTrack` moved similarly, but into a *different* new module,
`opencv2/features.hpp` (also new in 5.0, doesn't exist in 4.x). Same treatment: a second probe,
`HAVE_OPENCV2_FEATURES_HPP`, gates a guarded include in `js_imgproc.cpp`.

**Real removal in this group**: `linearPolar`/`logPolar` are gone with no replacement declared
anywhere in the 5.0 headers (they were already deprecated in 4.x in favor of `warpPolar`, which
has different parameter semantics — not a drop-in). Guarded out via `HAVE_OPENCV_LINEARPOLAR`/
`HAVE_OPENCV_LOGPOLAR` probes (declaration-only checks via `decltype`, see "CMake probe
technique" below) — both the `case` bodies and their `JS_CFUNC_MAGIC_DEF` registrations in
`js_imgproc.cpp` are removed on 5.x, so `cv.linearPolar`/`cv.logPolar` are simply `undefined`
from JS rather than throwing at call time.

### 2. `MatSize::dims()` → `MatShape::dims`

OpenCV 5.0 redesigned `cv::MatSize` as an alias for a new `cv::MatShape` struct (to support
tensor-shape-aware `dnn` code). In OpenCV 4.x, `MatSize::dims()` is a **method** and `MatSize` has
an implicit `operator const int*()`. In OpenCV 5.0, `MatShape::dims` is a plain **public int
field**, and the implicit pointer conversion is gone.

**Fix — fully portable, no `#ifdef` needed**: `cv::Mat`/`cv::UMat` themselves already expose their
own `int dims` field, unchanged across both versions (confirmed identical in both header sets).
`js_mat.cpp`/`js_umat.cpp`'s `size.dims()` calls were rewritten to use the enclosing Mat's own
`.dims` field instead. `algorithms/dominant_colors_grabber.cpp`'s `Mat` constructor call
(`Mat(3, hist.size, type, scalar)`, relying on the dropped implicit conversion) was rewritten to
`Mat(hist.dims, hist.size.p, type, scalar)` — `MatSize`/`MatShape` both expose a public `.p`
pointer member in both versions, so this compiles unchanged on 4.x and 5.x alike.

### 3. Chessboard calibration: `calib3d` → `objdetect`

`findChessboardCorners`, `findChessboardCornersSB`, `drawChessboardCorners`, and all
`CALIB_CB_*` constants moved from `calib3d`/`calib` into `objdetect` in OpenCV 5.0 (OpenCV 5's
`opencv2/calib3d.hpp` is now just a compatibility shim including `geometry.hpp` + `stereo.hpp` +
`calib.hpp`, none of which declare these — they live in `opencv2/objdetect.hpp` now, alongside
the other 2D-marker/code-detection functionality).

**Fix — fully portable, no `#ifdef` needed**: added `#include <opencv2/objdetect.hpp>` to
`js_calib3d.cpp`. Harmless no-op on 4.x (the declarations are already visible there via
`calib3d.hpp`, no redeclaration conflict since only one header declares them per version).

### 4. `AKAZE`/`BRISK`/`KAZE`/`AgastFeatureDetector`: `features2d` → `xfeatures2d`

These four detector classes moved out of the core `features2d`/`features` module into
`cv::xfeatures2d` (an opencv_contrib module) in OpenCV 5.0. `opencv2/features2d.hpp` on 5.0 is now
just a compatibility header that includes `opencv2/features.hpp`, which no longer declares these
four. This is a genuine relocation, not a removal — they still work identically, just require
`opencv_xfeatures2d` (which this project already links for `USE_FEATURE2D`).

**Fix**: `js_feature2d.cpp`'s `using cv::AKAZE;` etc. are now branched on a new
`HAVE_OPENCV_XFEATURES2D_AKAZE` probe: `using cv::xfeatures2d::AKAZE;` on 5.x,
`using cv::AKAZE;` on 4.x.

### 5. DNN engine rewrite

OpenCV 5.0 introduced a new DNN execution engine (`cv::dnn::dnn5_v20260605` inline namespace) as
an alternative to the classic one, selectable via `cv::dnn::EngineType`
(`ENGINE_CLASSIC`/`ENGINE_NEW`/`ENGINE_AUTO`/`ENGINE_ORT`). Several things landed in the same
header revision as `ENGINE_AUTO`, so one capability flag — `HAVE_OPENCV_DNN_NEW_ENGINE` — safely
gates all of them:

- **Genuinely removed, no replacement**: `cv::dnn::readNetFromCaffe`, `readNetFromDarknet`,
  `readNetFromTorch`, `readTorchBlob`, `shrinkCaffeModel` (the old Caffe/Darknet/Torch importers),
  `Net::getInputDetails`/`getOutputDetails`/`quantize`/`setHalideScheduler`, and
  `DNN_BACKEND_HALIDE` (the Halide backend was dropped entirely, consistent with
  `setHalideScheduler` going with it). All of the corresponding `case` bodies,
  `JS_CFUNC_MAGIC_DEF`/`JS_PROP_INT32_DEF` registrations in `js_dnn.cpp` are now wrapped in
  `#ifndef HAVE_OPENCV_DNN_NEW_ENGINE` — these JS functions/constants simply don't exist when
  built against 5.x, rather than compiling against a symbol that isn't there.
- **Relocated, not removed**: `cv::dnn::DataLayout` (and its `DNN_LAYOUT_*` enumerators) moved
  from being declared *inside* `namespace cv::dnn` (4.x) to the enclosing `cv` namespace (5.x,
  where it's shared with the new `MatShape`-aware layout concept). Because C++ qualified lookup
  (`cv::dnn::DataLayout`) only searches the named namespace, not its enclosing scope,
  `cv::dnn::DataLayout` stops resolving on 5.x even though the type still exists (as `cv::DataLayout`)
  and is still visible *unqualified* from inside `namespace dnn` via normal scope lookup — which is
  exactly why OpenCV's own `dnn.hpp` continues to compile unchanged while code outside it, like
  ours, breaks. This is also what caused the seemingly unrelated `'mode' was not declared in this
  scope` errors: the local-variable declaration on that line failed because its initializer used
  the now-invalid `cv::dnn::DNN_LAYOUT_NCHW`, so `mode` (declared on the same line) never came into
  existence — confirmed by isolating the file with `g++ -fsyntax-only` and reading the actual
  diagnostic. **Fix**: a small `qjs_dnn_compat` namespace shim near the top of `js_dnn.cpp`
  provides a version-independent `DataLayout`/`DNN_LAYOUT_*` spelling, backed by
  `HAVE_OPENCV_DNN_NEW_ENGINE`.
- **New capability added**: `dnn.ENGINE_CLASSIC`/`ENGINE_NEW`/`ENGINE_AUTO`/`ENGINE_ORT` are now
  exported to JS (guarded), and `dnn.readNetFromONNX(path[, engine])` accepts the optional
  `engine` argument on 5.x (defaulting to `ENGINE_AUTO`), matching the new
  `readNetFromONNX(path, engine)` overload OpenCV 5 added. On 4.x the binding is unchanged
  (2-arg max, no `engine` parameter exists to pass).

Runtime-verified (not just compile-verified): a smoke test run against both the 4.13.0 and 5.0.0
builds shows `dnn.ENGINE_AUTO` is `undefined` and `dnn.readNetFromCaffe` is present on 4.13.0, and
`dnn.ENGINE_AUTO === 3` with `dnn.readNetFromCaffe === undefined` on 5.0.0 — the guard correctly
switches behavior at the JS API surface, not just at compile time.

### 6. `BarcodeDetector` constructor signature change

OpenCV 4.x's `cv::barcode::BarcodeDetector(prototxt_path, model_path)` two-string constructor
(for loading a custom super-resolution model as a separate prototxt/weights pair) is gone in
OpenCV 5.0, replaced by a default constructor and a single-string
`BarcodeDetector(super_resolution_model_path)` constructor. **This is a genuine, if narrow,
capability loss**: OpenCV 5 no longer supports loading a super-resolution model as a
prototxt+weights pair from this API — only a single bundled model path.

**Fix**: guarded via `HAVE_OPENCV_BARCODE_LEGACY_CTOR` (probed via `decltype` overload
resolution against the old two-arg ctor, since a declaration/linkage check can't distinguish
overloads — see "CMake probe technique" below). On 4.x, the JS constructor keeps accepting
`(prototxtPath, modelPath)` unchanged. On 5.x, only the first argument (if given) is used, passed
as the single super-resolution model path; passing a second argument is silently ignored rather
than erroring, since there's no equivalent parameter to route it to.

### 7. `LineSegmentDetectorModes` enum strictness

`cv::createLineSegmentDetector`'s `refine` parameter is typed `cv::LineSegmentDetectorModes` in
both versions, but OpenCV 4.x's headers allowed an implicit `int → enum` conversion for this
argument (compiling with `-fpermissive`-style leniency in practice — actually just an unscoped
enum accepting int args). OpenCV 5.0 tightened this to a hard error.

**Fix — fully portable, no `#ifdef` needed**: `static_cast<cv::LineSegmentDetectorModes>(refine)`
at the call site in `js_line_segment_detector.cpp` — a no-op cast on 4.x, required on 5.x.

### 8. `opencv2/core/core_c.h` removed

OpenCV 5.0 dropped the legacy C API headers entirely, including `core_c.h`. `js_cv.cpp` included
it but only used `cvRound`/`cvFloor`/`cvCeil` from it, which actually live in
`opencv2/core/fast_math.hpp` — already pulled in transitively via `opencv2/core.hpp` on **both**
versions.

**Fix — fully portable, no `#ifdef` needed**: deleted the dead `#include`. This was also the
single **fatal** error in the original 144 — it stopped that translation unit's parsing early,
which is why `js_cv.cpp` initially showed only 1 error; once the fatal error was fixed, 9 more
(genuinely version-specific) errors in the same file became visible (see #9).

### 9. Deprecated `VideoCapture` backend constants + `convertFp16` removed

Once `js_cv.cpp` could parse past its `core_c.h` fatal error, ten more errors appeared, all
genuine removals: the `CAP_VFW`, `CAP_QT`, `CAP_UNICAP`, `CAP_OPENNI`, `CAP_OPENNI_ASUS`,
`CAP_GIGANETIX` enum constants (ancient/deprecated capture backends: Video for Windows,
QuickTime, uniCap, first-generation OpenNI, Smartek Giganetix) were dropped entirely in 5.0;
`CAP_PROP_GIGA_FRAME_HEIGH_MAX`/`CAP_PROP_GIGA_FRAME_SENS_HEIGH` (typo aliases OpenCV itself kept
around "for source compatibility" under `#if CV_VERSION_MAJOR <= 4`) are gone in 5.0 by OpenCV's
own design; and `cv::convertFp16` (deprecated in 4.x in favor of `Mat::convertTo` with `CV_16F`)
was fully removed.

**Fix**: `HAVE_OPENCV_CAP_VFW` and `HAVE_OPENCV_CONVERT_FP16` probes gate the six backend
constants and `convertFp16`'s binding respectively; the two typo-alias constants are gated
directly with `#if CV_VERSION_MAJOR <= 4`, mirroring OpenCV's own header guard exactly (no custom
CMake probe needed since OpenCV already defines this macro).

## CMake probe technique notes

Two probe styles were needed beyond the project's existing `check_include_file_cxx`/
`check_cxx_symbol_exists` idiom (see `HAVE_OPENCV_AFFINE_FEATURE` for the pre-existing pattern):

- **`check_cxx_symbol_exists` doesn't work for enum constants or type names** — its underlying
  test takes `&symbol`, which requires an addressable lvalue. `ENGINE_AUTO`, `CAP_VFW` (enum
  constants) and `AKAZE` (a type name) all fail this with compile errors even when the symbol
  exists. Switched to `check_cxx_source_compiles` with a small snippet that actually uses the
  symbol in a context that only needs its *type* (`int e = cv::dnn::ENGINE_AUTO;`,
  `cv::Ptr<cv::xfeatures2d::AKAZE> p;`).
- **Declaration-only checks avoid spurious link failures**: `linearPolar`, `logPolar`,
  `convertFp16`, and the `BarcodeDetector` legacy-ctor probe all originally tried to link a real
  call/construction, which pulled in `opencv_core`/`opencv_imgproc`/`opencv_objdetect` for the
  test executable. On this machine, the local OpenCV 4.11.0 build transitively depends on
  `libtbb.so.2`, which isn't installed — so these throwaway test executables failed to link with
  `undefined reference` to TBB symbols, even though the *real* project build (a shared library,
  which doesn't require full symbol resolution at link time the way an executable does) links
  fine. Switched all of these to `decltype`-based checks (`decltype(&cv::linearPolar) fp =
  nullptr;`, `decltype(cv::barcode::BarcodeDetector(std::declval<std::string>(), ...))`) which
  only need the *declaration* to compile, sidestepping linking entirely — appropriate here since
  the invariant that actually matters is "does the guarded call site have a declaration to compile
  against," and the real build's link step (already proven to work) covers the rest.

## Bugs found and fixed along the way

Two pre-existing, unrelated bugs in `CMakeLists.txt` were discovered while adding the new
capability probes — both were latent because they were masked by stale cache state in existing
build directories, and only surfaced once this work did a from-scratch configure. Full details
and repros are in `BUGS`; summary:

1. **`option(USE_FEATURE2D ...)`/`option(USE_BARCODE ...)` were declared after their first
   `if()` use** in `CMakeLists.txt`. On a from-scratch configure (no prior `CMakeCache.txt`), both
   variables were undefined at the point of use, so the `if(USE_FEATURE2D)`/`if(USE_BARCODE)`
   blocks were silently skipped entirely — `-DUSE_FEATURE2D=1`, the `HAVE_OPENCV2_XFEATURES2D_HPP`
   probe, `FEATURE2D_SOURCE`, etc. never happened. Fixed by moving both `option()` calls above
   their first use.
2. **A broken, unused `check_library_exists()` call for `BarcodeDetector`** in the (now
   correctly-reached) `if(USE_BARCODE)` block failed configuration outright with `No known
   features for CXX compiler` from inside `CheckLibraryExists.cmake`'s internal `try_compile`.
   Its result variable, `OPENCV_BARCODE`, was never read anywhere in the project (confirmed by
   grep) — dead code even when it worked. Removed rather than repaired, since nothing depends on
   it.

## Files changed

- **CMake**: `CMakeLists.txt` — new capability probes (§ above), plus the two bug fixes.
- **Portable, no-guard fixes**: `js_cv.cpp` (dead include), `js_mat.cpp`/`js_umat.cpp`
  (`.dims`), `algorithms/dominant_colors_grabber.cpp` (`Mat` ctor), `js_calib3d.cpp` (extra
  include), `js_line_segment_detector.cpp` (cast).
- **Guarded fixes**: `js_imgproc.cpp`, `js_contour.cpp`, `js_subdiv2d.cpp`, `src/geometry.cpp`
  (geometry module include), `js_feature2d.cpp` (xfeatures2d namespace), `js_dnn.cpp` (engine
  rewrite — the largest single change), `js_barcode_detector.cpp` (ctor), `js_cv.cpp` (legacy
  constants).
- **Docs**: `BUGS` (two new entries), this file.

## Verification

All three target build directories (`build/x86_64-linux-opencv411`, `-opencv413`, `-opencv500`,
configured against `/opt/opencv-4.11.0-x86_64`, `/opt/opencv-4.13.0-x86_64`, and
`/opt/opencv-5.0.0-x86_64` respectively) were wiped and reconfigured from scratch, then built
clean (`make -j$(nproc)`) with **zero errors each**. Capability-flag correctness was spot-checked
directly in each build's `CMakeCache.txt`:

| Flag | 4.11.0 | 4.13.0 | 5.0.0 |
|---|---|---|---|
| `HAVE_OPENCV2_GEOMETRY_HPP` | — | — | 1 |
| `HAVE_OPENCV2_FEATURES_HPP` | — | — | 1 |
| `HAVE_OPENCV_DNN_NEW_ENGINE` | — | — | 1 |
| `HAVE_OPENCV_XFEATURES2D_AKAZE` | — | — | 1 |
| `HAVE_OPENCV_BARCODE_LEGACY_CTOR` | 1 | 1 | — |
| `HAVE_OPENCV_CAP_VFW` | 1 | 1 | — |
| `HAVE_OPENCV_CONVERT_FP16` | 1 | 1 | — |
| `HAVE_OPENCV_LINEARPOLAR` / `_LOGPOLAR` | 1 | 1 | — |

A runtime smoke test (dynamically importing each build's `opencv.so` directly by path, since
`QUICKJS_MODULES` did not override the compiled-in module search path in this environment — a
pre-existing, unrelated qjs-runtime quirk, not part of this migration) confirmed, on both 4.13.0
and 5.0.0:

- `Contour.getBoundingClientRect()`/`.area` (the geometry-module relocation) return identical,
  correct results (`width=10`, `area=100`) on both versions.
- `dnn.ENGINE_AUTO`/`dnn.readNetFromCaffe` correctly flip between "classic engine, legacy
  importers present" (4.13.0) and "new engine, `ENGINE_AUTO=3`, legacy importers absent" (5.0.0)
  at the actual JS API surface, not just at compile time.

The 4.11.0 build could not be runtime-smoke-tested in this sandbox — it fails to `dlopen` with
`libtbb.so.2: cannot open shared object file`, an environment limitation (TBB isn't installed
here), unrelated to this migration; the build itself is unaffected and compiles/links cleanly.

## Open items for follow-up (not blocking)

- **`linearPolar`/`logPolar` on 5.x**: currently just absent from the JS API (`typeof
  cv.linearPolar === 'undefined'`). Could be reimplemented in terms of `warpPolar`, but the
  parameter semantics differ enough (center/maxRadius/flags vs. dsize/center/maxRadius/flags)
  that it isn't a mechanical substitution — worth a deliberate decision before investing in it.
- **`BarcodeDetector` custom prototxt/model loading on 5.x**: no longer possible via this API at
  all (§6). If any caller actually relies on loading a custom super-resolution model as a
  prototxt+weights pair (as opposed to the single bundled-model-path form), that functionality
  needs a different OpenCV 5 API to be identified, or is simply gone.
- **`AKAZE`/`BRISK`/`KAZE`/`AgastFeatureDetector` on 5.x require `opencv_xfeatures2d`** to be
  present at build time (this project already requires it for `USE_FEATURE2D`, so no new
  dependency in practice, but worth noting if `USE_FEATURE2D` is ever made independently
  toggleable from contrib availability).
