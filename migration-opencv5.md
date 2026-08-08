# OpenCV 5.0 migration

## Functionality actually lost going from 4.13.0 to 5.0.0

Everything else in this document is a relocation or a call-site detail change — same capability,
different spelling. This section is the opposite: the specific, narrow set of things that OpenCV
5.0 genuinely dropped, with no drop-in replacement, and how each is handled in the qjs-opencv
bindings as a result.

| Lost | What it did | Replacement in 5.0? | How it's handled here |
|---|---|---|---|
| `cv.linearPolar` / `cv.logPolar` | Cartesian↔polar image remapping | No — `warpPolar` exists but has different parameter semantics (dsize-based, not a mechanical substitute) | Bindings compiled out on 5.x (`typeof cv.linearPolar === 'undefined'`); reimplementing atop `warpPolar` is an open follow-up, not done |
| `BarcodeDetector` custom prototxt+weights loading | `new BarcodeDetector(prototxtPath, modelPath)` — load an arbitrary custom super-resolution model as a separate architecture/weights pair | No — only `new BarcodeDetector(modelPath)` (single bundled model file) or the no-arg default ctor remain | Guarded: on 5.x, only the first constructor argument is used (as the single model path); a second argument is silently ignored rather than erroring |
| `dnn.readNetFromCaffe` / `readNetFromDarknet` / `readNetFromTorch` / `readTorchBlob` / `shrinkCaffeModel` | Importing Caffe, Darknet, and Torch model formats into `cv.dnn.Net` | No — these formats have no importer at all in OpenCV 5's DNN module | Bindings compiled out on 5.x; only ONNX/TensorFlow/TFLite/model-optimizer import remain |
| `Net.getInputDetails` / `Net.getOutputDetails` | Reading per-tensor quantization scale/zero-point metadata off a loaded net | No | Compiled out on 5.x |
| `Net.quantize` | Post-training quantization of a loaded classic-engine net | No — quantization is handled differently in the new DNN engine, not exposed as an equivalent `Net` method | Compiled out on 5.x |
| `Net.setHalideScheduler` and the Halide backend (`dnn.DNN_BACKEND_HALIDE`) | Selecting Halide as the DNN inference backend | No — the Halide backend was removed from OpenCV 5 entirely | Both compiled out on 5.x |
| `cv.convertFp16` | Converting a `Mat` to/from 16-bit float | Yes, but not a drop-in — use `mat.convertTo(dst, CV_16F)` (already bound generically) instead of a dedicated function | Binding compiled out on 5.x since the underlying `cv::convertFp16` symbol is gone; callers need to switch to `convertTo` |
| Legacy `VideoCapture` backend constants: `CAP_VFW`, `CAP_QT`, `CAP_UNICAP`, `CAP_OPENNI` (v1), `CAP_OPENNI_ASUS`, `CAP_GIGANETIX` | Selecting long-deprecated capture backends (Video for Windows, QuickTime, uniCap, first-gen OpenNI, Smartek Giganetix) | No — these backends were dropped from OpenCV 5 itself, not just deprecated | Constants compiled out on 5.x; `cv.VideoCapture` still works normally with any backend OpenCV 5 still supports |
| `AKAZE` / `BRISK` / `KAZE` / `AgastFeatureDetector` as core `features2d` classes | Feature detectors usable without opencv_contrib | Yes, functionally — moved into `cv::xfeatures2d` (contrib), same behavior | Not a loss in *this* project specifically (already builds with contrib for `USE_FEATURE2D`), but would be a loss for anyone building without contrib |

Two items above are footnotes rather than real losses and are omitted from the table: the
`CAP_PROP_GIGA_FRAME_HEIGH_MAX`/`CAP_PROP_GIGA_FRAME_SENS_HEIGH` constants are pure typo aliases
OpenCV itself dropped in 5.0 — the correctly-spelled `CAP_PROP_GIGA_FRAME_HEIGHT_MAX`/
`CAP_PROP_GIGA_FRAME_SENS_HEIGHT` remain fully available and unchanged.

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

**Fix — unified JS API**: guarded via `HAVE_OPENCV_BARCODE_LEGACY_CTOR` (probed via `decltype`
overload resolution against the old two-arg ctor, since a declaration/linkage check can't
distinguish overloads — see "CMake probe technique" below), but the JS-visible constructor now
accepts the *same three call shapes on both versions*, mapped onto whichever native ctor is
actually available:

- `new BarcodeDetector()` — no model, identical on both.
- `new BarcodeDetector(modelPath)` — the single-file form. Native on 5.x. On pre-5.x, where no
  single-file ctor exists, this now throws a clear, catchable `TypeError` explaining the
  version requirement, instead of silently doing something surprising.
- `new BarcodeDetector(prototxtPath, modelPath)` — the two-file form. Native on pre-5.x. On 5.x,
  only `modelPath` (the actual weights — the substantive artifact of the pair) is used;
  `prototxtPath` is ignored rather than erroring, since 5.x has no separate-architecture-file
  concept to route it to.

In every shape, "the last argument given" is always the model path, so code that always passes
the model path last works unchanged regardless of version. A new `BarcodeDetector.LEGACY_CTOR`
static boolean (`true` pre-5.x, `false` on 5.x) lets callers introspect which native ctor is
backing the two-file form, without needing a separate OpenCV-version check. Covered by
`tests/test_barcode.js`.

**Bug found and fixed while unifying this**: `js_barcode_detector_method()` (the dispatcher
behind `detect`/`decodeWithType`/`detectAndDecodeWithType`) had no `try/catch` at all, unlike
every other method dispatcher in the codebase. Any `cv::Exception` OpenCV's barcode detector
threw — confirmed to happen on both 4.13.0 and 5.0.0 for certain degenerate inputs — crashed the
whole `qjs` process via `std::terminate()` instead of raising a catchable JS exception. Fixed by
wrapping the dispatcher in the same `try { ... } catch(const cv::Exception& e) { ret =
js_cv_throw(ctx, e); }` pattern used elsewhere (e.g. `js_dnn.cpp`'s `js_net_method`). Logged in
`BUGS` as `js-barcode-detector-method-crashes-process-on-cv-exception`.

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
| `HAVE_OPENCV_DNN_TOKENIZER` | — | — | 1 |
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

## New in OpenCV 5.0 that could be bound (not a migration blocker, ranked by leverage)

Found by diffing exported (`CV_EXPORTS`/`CV_WRAP`) declarations between
`/opt/opencv-4.13.0-x86_64/include/opencv4` and `/opt/opencv-5.0.0-x86_64/include/opencv5`. None
of this is required for the migration — it's new capability 5.0 adds on top, listed for anyone
deciding what to bind next. Highest-leverage first:

1. **Point cloud / mesh I/O — `opencv2/ptcloud.hpp`, new `libopencv_ptcloud.so`.** Wholly new
   module: `loadPointCloud`/`savePointCloud`, `loadMesh`/`saveMesh` (PLY/OBJ/etc.),
   `triangleRasterize`/`triangleRasterizeDepth`/`triangleRasterizeColor`, and a `CV_EXPORTS_W
   Octree` class (`Octree::createWithDepth(...)` from either explicit depth/size or a point
   cloud, insert/query/radius-search). Highest leverage because it's a capability gap this project
   has none of today (no point-cloud or mesh I/O at all), it's a single self-contained new
   `js_ptcloud.cpp` following the existing one-file-per-concept convention, and the module has no
   unusual runtime dependencies beyond itself.
2. **Learned local features — `opencv2/features.hpp`.** `DISK` and `ALIKED` (learned
   keypoint detector/descriptors, `Ptr<DISK> DISK::create(modelPath, ...)`, ONNX-backed via the
   `dnn` module — `create(bufferModel, ...)` in-memory overload also available),
   `LightGlueMatcher` (a modern learned matcher, meant to pair with DISK/ALIKED, likely a strict
   upgrade over `BFMatcher`/`FlannBasedMatcher` for anyone doing feature matching/stitching), and
   `ANNIndex` (approximate nearest-neighbor index). High leverage for matching/stitching/SLAM-style
   pipelines, but each detector needs a model file the caller must supply (not bundled) and pulls
   in `HAVE_OPENCV_DNN` at the OpenCV build level — worth confirming that's on before committing to
   binding these.
3. **`opencv2/geometry.hpp` additions**: `minEnclosingConvexPolygon()` (reduce a convex polygon to
   k vertices) and `getClosestEllipsePoints()` (per-point fit error against an ellipse, companion
   to `fitEllipse`/`fitEllipseAMS`/`fitEllipseDirect`, already bound). Small, cheap wins — same
   header already included for the `HAVE_OPENCV2_GEOMETRY_HPP` migration fix (§1), natural fits
   next to the existing contour/shape functions in `js_contour.cpp`/`js_imgproc.cpp`.
4. **Multi-camera calibration — `opencv2/calib.hpp`**: `registerCameras()` (rig-level multi-camera
   calibration) and a new top-level `calibrate()` overload taking `InputOutputArray K, D`
   directly. Narrower audience (multi-camera rig setups specifically) than the above, but a real
   gap — `js_calib3d.cpp` currently only exposes single-camera `calibrateCamera`.
5. ~~**DNN text/LLM primitives**~~ — **done**, see "Wrapping LLM inference (Qwen2.5)" below.

Not investigated further / ruled out:
- `opencv2/xstereo.hpp` exists but declares no `CV_EXPORTS`/`CV_WRAP` symbols at all — an empty
  compatibility stub, nothing to bind.
- A `diff` of `CV_WRAP` methods in `core/mat.hpp` initially flagged `channels()`, `clear()`,
  `empty()`, `erase()`, `expand()`, `hasSymbols()`, `isScalar()`, `toLayout()` as new — these all
  belong to the new `MatShape` struct (already covered as the `MatSize::dims()` relocation, §2),
  not to `cv::Mat`/`cv::UMat` themselves. False lead, excluded here.
- `Model`/`ClassificationModel`/`DetectionModel`/`SegmentationModel`/`KeypointsModel`/
  `TextRecognitionModel`/`TextDetectionModel*` in `dnn.hpp` are not new to 5.0 (present in 4.13
  too) — **now bound**, see "Wrapping the classic DNN convenience Model classes" below.

## Wrapping LLM inference (Qwen2.5)

OpenCV 5.0 added exactly two new DNN primitives aimed at LLM-style autoregressive inference —
`cv::dnn::Tokenizer` and `Net::enableKVCache()`/`disableKVCache()`/`resetKVCache()` — plus the
existing `ENGINE_ORT` (§5) as the practical way to actually execute a transformer exported to
ONNX. There is no built-in generation loop, sampler, or chat template anywhere in OpenCV; running
something like Qwen2.5 end-to-end means composing these primitives from JS:

1. `dnn.readNetFromONNX(path, dnn.ENGINE_ORT)` (or `ENGINE_AUTO`, which tries ORT and falls back)
   to load the exported model. This project's OpenCV 5.0.0 build already links
   `libonnxruntime.so.1` (1.25.1, confirmed via `ldd` on `libopencv_dnn.so`) transitively through
   `opencv_dnn`, so `ENGINE_ORT` works with no extra linking on the qjs-opencv side.
2. `net.enableKVCache()` once, before the generation loop, so attention layers reuse
   past keys/values across steps instead of recomputing the whole sequence every token.
3. `dnn.Tokenizer.load(configJsonPath)` to get a tokenizer, `tok.encode(text)` to turn the prompt
   into token ids, feed those through `net.setInput(...)`/`net.forward()` per step (there is no
   `generate()` helper — the sampling loop, whatever it is: greedy, temperature, top-k/top-p, is
   plain JS around repeated `forward()` calls), and `tok.decode(ids)` to turn generated ids back
   into text.
4. `net.resetKVCache()` between independent generations reusing the same `Net`.

All of this is now bound: `dnn.Tokenizer` (`.load(path)` static factory, `.encode(text)` →
`Int32Array`, `.decode(tokens)` → `string`) and `Net.prototype.{enable,disable,reset}KVCache()`,
gated by a dedicated `HAVE_OPENCV_DNN_TOKENIZER` CMake probe (declaration-only, same `decltype`
technique as the rest of this file) rather than reusing `HAVE_OPENCV_DNN_NEW_ENGINE` — the two
landed in the same header revision in this build but aren't guaranteed to always track each other.
`Tokenizer` is undefined and the three KV-cache methods are absent from `Net.prototype` on pre-5.x
builds. Verified with a real end-to-end `encode()`/`decode()` round-trip against Qwen2.5's actual
tokenizer (downloaded from HuggingFace, committed at `tests/qwen2.5-tokenizer/`) — see
`tests/test_tokenizer.js`.

**`Tokenizer::load()`'s doc comment in dnn.hpp is misleading — reading OpenCV's actual
`modules/dnn/src/tokenizer/tokenizer.cpp` (this machine happens to have the OpenCV source tree
checked out) was necessary to get this working at all:**

- The doc comment describes the argument as a directory ("`Tokenizer::load("/path/to/model/")`").
  It's actually **the path to `config.json` itself** — `Tokenizer::load()` opens that path
  directly as a `cv::FileStorage`, then derives the directory `tokenizer.json` lives in by
  stripping the filename back off whatever path was given. Passing a directory instead (as the
  doc comment suggests, and as this project's binding originally assumed) makes `cv::FileStorage`
  try to open the directory itself as a file, which trips `CV_Assert(buf)` deep in
  `opencv2/core/persistence.cpp` with an unhelpful `Assertion failed: buf in function 'open'` —
  still a normal catchable `cv::Exception`, not a crash, just a confusing one to debug blind.
- The doc comment says `model_type` must be `"gpt2"` or `"gpt4"`. The actual implementation also
  accepts (and, for a real Qwen tokenizer.json, *requires*) `"qwen2"`/`"qwen2.5"` — using `"gpt2"`
  against Qwen2.5's vocabulary silently selects the wrong regex-split pattern rather than failing,
  which would have produced mis-tokenized output instead of an error.

Both are now correct in `tests/qwen2.5-tokenizer/config.json` (`{"model_type": "qwen2.5"}`) and in
`tests/test_tokenizer.js` (`dnn.Tokenizer.load('qwen2.5-tokenizer/config.json')`), and real
Qwen2.5 tokenization round-trips correctly end-to-end. Not a qjs-opencv bug and not logged to
`BUGS` — the exception was always a normal, catchable `cv::Exception`, and the underlying cause
was this project calling the API the way its own (outdated) doc comment describes rather than the
way it actually works.

Separately, a real bug *was* found and fixed while testing this: `Net::enableKVCache()`/
`resetKVCache()` **segfaulted** (not a catchable `cv::Exception`) when called on a `Net` with no
layers loaded — confirmed on a plain `new dnn.Net()`. The crash is inside OpenCV's own
implementation, but `Net::empty()` (already bound as `Net.prototype.empty`) reliably reports
whether a net actually has layers, so `js_net_method()` now checks it before forwarding either
call and throws a plain catchable `TypeError` instead. `disableKVCache()` alone was already safe
on an empty net and needed no guard. See `tests/test_tokenizer.js`.

## Wrapping the classic DNN convenience Model classes

`Model` and its seven typed subclasses (`ClassificationModel`, `DetectionModel`,
`SegmentationModel`, `KeypointsModel`, `TextRecognitionModel`, `TextDetectionModel_EAST`,
`TextDetectionModel_DB`) have existed unchanged since well before 4.13 but were never bound in
`js_dnn.cpp` — a pre-existing gap unrelated to the 5.0 migration, closed while working on it since
the header-diffing pass surfaced it. All eight share an identical two-shape construction API
(`new X(net)` from an existing `dnn.Net`, or `new X(model[, config])` from files) and the same
`Model`-inherited proxy methods (`setInputSize`/`setInputMean`/`setInputScale`/`setInputCrop`/
`setInputSwapRB`/`setOutputNames`/`setInputParams`/`predict`/`setPreferableBackend`/
`setPreferableTarget`/`enableWinograd`) — bound once via a shared C++ template (`js_model_base_method`)
and a proto-funcs macro (`DNN_MODEL_BASE_PROTO_FUNCS`) reused by all eight classes instead of
duplicating that logic seven times, plus a `DEFINE_DNN_MODEL_SKELETON` macro for the identical
class-id/proto/ctor/finalizer boilerplate every one of them needs. `TextDetectionModel_EAST`/`_DB`
additionally share `detect()`/`detectTextRectangles()` through a second, smaller template
(`js_text_detection_model_method`) mirroring their common (unbound — protected constructor, not
directly constructible) `TextDetectionModel` base. Unlike everything else in this file, none of
this needs a version guard: the API is byte-identical across 4.13 and 5.0.

No real model weights are available in this environment, so `tests/test_model.js` and its seven
siblings (`test_classification_model.js`, `test_detection_model.js`, `test_segmentation_model.js`,
`test_keypoints_model.js`, `test_text_recognition_model.js`, `test_text_detection_model_east.js`,
`test_text_detection_model_db.js`) exercise the JS binding surface itself rather than real
inference: construction from both an empty `dnn.Net` and a bogus file path (expecting a catchable
throw, not a crash — the same standard `tests/test_barcode.js` established), setter/getter
round-trips, and calling `predict()`/`classify()`/`detect()`/etc. on an empty network (expecting a
catchable `cv::Exception`, verified on both 4.13.0 and 5.0.0). All fifteen new/updated test files
pass on both builds.
