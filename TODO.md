# TODO — binding backlog

Prioritized by leverage for plot-cv's actual pipelines — laser-cutting SVG export, and three planned vectorization examples (`examples/sticker2svg.js`, `examples/collage-art.js`, `examples/vector-trace.js`) — not by raw OpenCV API surface. Derived from a `scripts/binding_coverage.js` run against `build/x86_64-linux-debug/opencv.so` (OpenCV 4.13.0), filtered to `cv::`-namespace symbols only.

Regenerate the underlying data with:
```bash
qjsm scripts/binding_coverage.js --module=build/x86_64-linux-debug/opencv.so \
  --lib-dir=/opt/opencv-4.13.0-x86_64/lib --namespace=cv --verbose --out=cov.txt
```

## opencv.js Example Compatibility Gaps (2026-08-17)

Cross-checked every `cv.*` binding referenced by the 80 opencv.js example
pages cataloged in `doc/opencv-js-examples.md` (source: local checkout
`/mnt/data/Projects/opencv/doc/js_tutorials/js_assets/*.html`) against this
project's actual live exports (`Object.keys(cv)` from a built
`build/x86_64-linux-gnu/opencv.so`, OpenCV 5.0.0). 158 unique `cv.*`
symbols are referenced across all 80 pages; 142 already resolve. The 16
gaps below block one or more example pages from running unmodified; full
detail (repro, exact source locations) for each is filed as its own entry
in `BUGS` under the `opencvjs-*` canonical-name prefix.

**Highest impact - fix first:**

- **`.delete()` missing on `Mat`/vector-wrapper classes** - not a binding
  gap, an architectural difference (QuickJS GC + finalizers vs opencv.js's
  manual wasm-heap `.delete()`) - but it's called **349 times** across the
  80 pages, more than any other single symbol, so every unmodified example
  hits it immediately. A harmless no-op `.delete()` stub on `Mat` (and the
  `*Vector` classes) would fix all 80 pages' single biggest compatibility
  blocker in one change, without touching the GC design.
  See `BUGS: opencvjs-mat-delete-missing`.

**New binding work, by tutorial category:**

- `js_video` (5 example pages, all blocked): no `opencv2/video.hpp`
  tracking module bound at all - `cv.CamShift`, `cv.meanShift`,
  `cv.calcOpticalFlowPyrLK`, `cv.calcOpticalFlowFarneback` are all
  unimplemented, plus the `cv.TermCriteria` constructor two of them need.
  All four C++ functions are `CV_EXPORTS_W` in the local OpenCV headers -
  pure binding work, no missing dependency. Proposed: a new `js_video.cpp`
  mirroring the existing `js_calib3d.cpp`/`js_dnn.cpp` module pattern.
  See `BUGS: opencvjs-video-tracking-module-unbound`,
  `opencvjs-termcriteria-constructor-missing`.
- `js_imgproc` (5 example pages blocked): `cv.matchTemplate` +
  `cv.TM_*` constants (`js_template_matching_matchTemplate.html`),
  `cv.getOptimalDFTSize` (`js_fourier_transform_dft.html`),
  `cv.calcBackProject` (`js_histogram_backprojection_calcBackProject.html`,
  plus 2 of the `js_video` pages above), `cv.DIST_*` constants (3
  `js_watershed_*` pages - `cv.distanceTransform()` itself is already
  bound and takes this parameter, it just can't be named from JS),
  `cv.segmentation_IntelligentScissorsMB` (`js_intelligent_scissors.html`
  - class exists in the local checkout's `opencv2/photo/segmentation.hpp`,
  and `js_photo.cpp` already exists as the place to add it).
  See `BUGS: opencvjs-matchtemplate-missing`, `opencvjs-tm-constants-missing`,
  `opencvjs-getoptimaldftsize-missing`, `opencvjs-calcbackproject-missing`,
  `opencvjs-dist-constants-missing`, `opencvjs-intelligentscissors-missing`.
- `js_core`/browser-helper layer: `cv.matFromArray(rows, cols, type,
  array)` has no equivalent top-level function (closest match: `new
  Mat(rows, cols, type, typedArray.buffer)`, already flagged inline at
  `js_mat.cpp:386`). See `BUGS: opencvjs-matfromarray-missing`.

**Shape/naming mismatches (functionality exists, calling convention doesn't match):**

- `cv.ellipse1(img, rotatedRect, color, thickness, lineType)` - the
  RotatedRect-taking overload of `ellipse()` - isn't implemented;
  `js_draw_ellipse` only parses the (center, axes, angle) shape. Blocks
  `js_contour_features_fitEllipse.html`.
- `cv.rotatedRectPoints(rect)` doesn't exist as a free function; the same
  functionality is `rotatedRect.points()` (instance method). Blocks
  `js_camshift.html` as written (one-line fix to adapt).
- `cv.BackgroundSubtractorMOG2` isn't directly constructible
  (`new cv.BackgroundSubtractorMOG2(...)`); only available via the
  `cv.createBackgroundSubtractorMOG2(...)` factory function under a
  generic `BackgroundSubtractor` class. Blocks `js_bg_subtraction.html` as
  written (one-line fix to adapt).

See `BUGS: opencvjs-ellipse-rotatedrect-overload-missing`,
`opencvjs-rotatedrectpoints-free-function-missing`,
`opencvjs-bgsubtractormog2-class-shape-mismatch`.

**Not applicable - don't implement:** `cv.FS_createDataFile` is
Emscripten's in-browser virtual filesystem, used by the 9 DNN examples
purely to stage a downloaded model where wasm can see it. qjs-opencv reads
model files directly off a real filesystem - the 9 DNN pages need this
call deleted when adapted, not replicated. See `BUGS:
opencvjs-fs-createdatafile-not-applicable`.

## Recent fixes (2026-08-17)

- **`Rect.contour()` crash fix.** `js_rect.cpp` `FUNC_CONTOUR` called `JSConverter<std::vector<cv::Point2d>>::toJS(ctx, c)` — no such specialization exists in `include/js_converter.hpp` (only individual `cv::Point`/`Point2f`/`Point3f` and a handful of other types are specialized, no generic `std::vector<T>` case), so this didn't compile. Fixed to use `js_array_from(ctx, c)`, matching the pattern already used two cases above in the same function and for `rects` elsewhere in the file; `js_value_from(const cv::Point_<T>&)` (`js_point.hpp:306`) covers the per-point conversion generically.
- **`EdgeDrawing.getSegments()` use-after-return.** `js_ximgproc.cpp` `EDGEDRAWING_GETSEGMENTS` stack-allocated `JSVector<std::vector<cv::Point>> pvv;` then called `pvv.toJS(ctx)`, which stashes `&pvv` as the returned JS object's opaque pointer. `JSVector<T>::finalizer` (`js_vector.hpp`) later `delete`s that opaque pointer from the GC — since it pointed at a since-popped stack frame, this was undefined behavior on every call. Fixed by heap-allocating `pvv` with `new`.
- **`new Mat(vectorInstance)` fixed.** `js_mat.cpp`'s `array.isVector()` branch (`js_mat_initialize`) used to pass `array.getObj()` — the address of the `std::vector<T>` container object itself — as the raw pixel-data pointer to `cv::Mat(size, type, data)`, instead of going through `array.getMat()`. Now reads `new(m) cv::Mat(array.size(), array.type()); array.getMat().copyTo(*m); return TRUE;` — `getMat()` builds the correctly-typed Mat header via OpenCV's own erased-type dispatch, so no hardcoded element type is needed at the call site. See `BUGS: mat-ctor-vector-getobj-as-data-pointer` (marked FIXED). Still untested from JS — worth a regression test (`new cv.PointVector()` → push points → `new cv.Mat(pv)`).

## Already solved, don't rebuild

- **Contour → SVG bezier splines.** No OpenCV algorithm does this (checked the full coverage survey — no such symbol exists, bound or unbound). It doesn't need a library either: `js/cvVectorization.js` (uncommitted) already has a correct Schneider/Graphics-Gems `FitCurves` cubic-bezier fitter (`CurveFitter` — corner detection, chord-length parameterization, Newton-Raphson reparameterization, recursive error-based subdivision) consuming `contour.array` directly, plus `SvgBuilder` for multi-region path output with holes via `fill-rule="evenodd"`. Keep this in JS; it's O(n) per subdivision and QuickJS handles it fine. Only reconsider a C++ port if profiling on a real workload shows it's the bottleneck.
- **Polyline simplification.** `cv.Contour` already exposes all seven `psimpl` algorithms as methods. **Migrated to freestanding functions** in `cv.psimpl.*` namespace (cv.psimpl.douglasPeucker, cv.psimpl.reumannWitkam, etc.) for opencv.js compatibility. See `js_psimpl.cpp` and Phase 1 completion notes.
- **Skeleton tracing.** `algorithms/skeleton_lines.hpp` (Guo-Hall thinning + topology-aware tracing that cuts at junctions) is fully bound: `skeletonizeGuohall`, `traceLines`, `degreeMap`, `skeletonizeAndTrace`. Distinct from `findContours` (region boundaries) and `LineSegmentDetector`/`FastLineDetector` (straight-line detection) — the three don't substitute for each other, pick per input character.
- **Region proposal for collage-art's "several algorithms to pick a motive."** `grabCut` (bound) plus `ximgproc::segmentation` selective-search/graph-segmentation (already bound) cover this with zero new binding work.

## Contour Migration Strategy

### Overview
Migrating from `Contour` class to opencv.js-compatible API through a phased approach. **Method migration is primary focus**; MatVector implementation is secondary and may not be 100% complete.

### Phase 1: Method Migration ✓ COMPLETE

**Goal:** Migrate Contour methods to freestanding functions for opencv.js compatibility

**Status:** ✓ COMPLETE (2026-08-13)
**Priority:** HIGH - enables existing opencv.js code to work

**Completed:**
- ✓ All 16 shape analysis functions already available as freestanding functions:
  contourArea, arcLength, boundingRect, approxPolyDP, convexHull,
  fitEllipse, fitLine, isContourConvex, minAreaRect, minEnclosingCircle,
  minEnclosingTriangle, pointPolygonTest, rotatedRectangleIntersection,
  convexityDefects, matchShapes, HuMoments

- ✓ All 7 psimpl methods migrated to cv.psimpl.* namespace:
  cv.psimpl.reumannWitkam, cv.psimpl.opheim, cv.psimpl.lang,
  cv.psimpl.douglasPeucker, cv.psimpl.nthPoint, cv.psimpl.radialDistance,
  cv.psimpl.perpendicularDistance

- ✓ psimpl functions accept: Mat CV_32SC2, Contour, or JS arrays
- ✓ psimpl functions return: Mat CV_32SC2 (opencv.js compatible)

**Testing:**
- tests/unittests/test_contour_functions.js: 16 tests for shape analysis ✓
- tests/unittests/test_psimpl_functions.js: 9 tests for psimpl functions ✓
- All tests pass with Mat CV_32SC2 data (findContours output format)

**Implementation:**
- Added js_psimpl_simplify() freestanding function in js_contour.cpp
- Uses magic dispatch for 7 simplification algorithms
- Supports Mat CV_32SC2 (zero-copy from findContours), Contour (backward compat), JS arrays
- Exported as cv.psimpl namespace (like cv.dnn)

**Impact:** Existing opencv.js code can now use freestanding functions with Mat CV_32SC2 data, enabling opencv.js compatibility without requiring MatVector implementation (Phase 2 is now lower priority).

**Drawing functions (already complete):**
- circle, line, rectangle, polylines, fillPoly (renamed from drawCircle, drawLine, etc.)

### Phase 2: MatVector Implementation (SECONDARY - Planned)
Implement MatVector class for running opencv.js code that uses it.

**Status:** Planned
**Priority:** MEDIUM - nice to have, enables more opencv.js compatibility
**Note:** Won't be 100% complete - some functions may still use Contour internally

**Research complete (2026-08-13):**
- Found 16 vector container types in opencv.js (MatVector, PointVector, KeyPointVector, DMatchVector, RectVector, etc.)
- All use common API: constructor, push_back, get, set, size, delete
- See BUGS entry `opencvjs-vector-containers` for full API spec

**Technical foundation (verified):**
- `findContours` with `vector<Mat>` output works
- Each contour Mat is `CV_32SC2`, Nx1 (rows=point count, cols=1)
- Zero-copy from native output, no int32→double conversion
- PointVector (vector<Vec2i>) NOT viable for Contours (findContours constraint)
- See BUGS for detailed feasibility analysis

**Implementation Plan:**
1. Design `js_vector.hpp` template infrastructure (see Phase 2A below)
2. Implement MatVector as first vector type
3. Gradually add other vector types as needed

**Implementation:** See BUGS entry `no-opencvjs-matvector` for API design

### Phase 2A: Vector Template Infrastructure Design

**Goal:** Create common template infrastructure for all 16 vector container types

**Status:** ✓ COMPLETE (2026-08-13)
**Priority:** HIGH - enables efficient implementation of all vector types

**Implementation:**
- ✓ Created `include/js_vector.hpp` with generic JSVector<T> template class
- ✓ Implemented JSConverter specializations for all types:
  - ✓ Primitive types: int, float, double, char, std::string
  - ✓ OpenCV types: cv::Mat, cv::Point, cv::Point2f, cv::Rect, cv::KeyPoint, cv::DMatch
- ✓ Added js_register_vector<T>() helper for type registration
- ✓ Added js_export_vector<T>() and js_set_vector_export<T>() for proper module export ordering
- ✓ Common operations: constructor, push_back, get, set, size, delete
- ✓ Symbol.iterator support for all vector types
- ✓ Automatic memory management via finalizers (GC-friendly, no manual delete required)

**Implemented Vector Types (11/16):**

HIGH Priority (all implemented):
- ✓ MatVector - vector<Mat> for findContours, split, merge, calcHist, aruco
- ✓ PointVector - vector<Point> for general point collections
- ✓ PointVectorVector - vector<vector<Point>> for nested point collections (findContours alternative output)
- ✓ KeyPointVector - vector<KeyPoint> for ORB, FAST, GFTT feature detection
- ✓ DMatchVector - vector<DMatch> for BFMatcher, drawMatches
- ✓ RectVector - vector<Rect> for rectangle collections

MEDIUM Priority (implemented):
- ✓ IntVector - vector<int> for channels, histSize parameters
- ✓ FloatVector - vector<float> for ranges parameters
- ✓ DoubleVector - vector<double> for precision-critical values
- ✓ CharVector - vector<char> for binary data
- ✓ StringVector - vector<string> for text collections

NOT YET IMPLEMENTED (low priority, rarely used):
- ⏳ Point2fVector - vector<Point2f> for sub-pixel precision
- ⏳ Point3fVector - vector<Point3f> for 3D points
- ⏳ DMatchVectorVector - vector<vector<DMatch>> for knnMatch
- ⏳ KeyPointVectorVector - vector<vector<KeyPoint>> for multi-image detection
- ⏳ CharVectorVector - vector<vector<char>> for nested binary data

**Testing:**
- ✓ test_all_vectors.js: 10/10 tests passing
- ✓ All basic operations verified: constructor, push_back, get, set, size, iterator
- ✓ Memory management verified: automatic cleanup via finalizers

**Key Design Decisions:**
- GC-friendly: No manual delete() required (unlike opencv.js)
- Value semantics for primitive and struct types (Point, Rect, KeyPoint, DMatch)
- Reference semantics for Mat (shared underlying data via refcount)
- Symbol.iterator support for `for (const item of vector)` syntax
- Automatic type conversion via JSConverter specializations

**See BUGS entry `opencvjs-vector-containers` for:**
- Complete API specification for all 16 vector types
- Memory management patterns and gotchas
- Usage examples from opencv.js tests
- Function parameter mappings (InputArrayOfArrays, OutputArrayOfArrays)
**Implementation Files:**
- include/js_vector.hpp - Core template infrastructure
- js_matvector.cpp - MatVector implementation
- js_pointvector.cpp - PointVector implementation
- js_rectvector.cpp - RectVector implementation
- js_keypointvector.cpp - KeyPointVector implementation
- js_dmatchvector.cpp - DMatchVector implementation
- js_primitivevectors.cpp - IntVector, FloatVector, DoubleVector, CharVector, StringVector

**See also:**
- BUGS: `opencvjs-vector-containers` for full API spec
- BUGS: `no-opencvjs-matvector` for MatVector-specific design

### Phase 2B: findContours Integration ✓ COMPLETE

**Goal:** Make findContours work with MatVector and PointVectorVector as output arrays

**Status:** ✓ COMPLETE (2026-08-13)
**Priority:** HIGH - enables zero-copy performance with opencv.js-compatible API

**Implementation:**
- ✓ Modified js_cv_find_contours to detect output array type:
  - MatVector (vector<Mat>) - zero-copy, each contour as CV_32SC2 Mat
  - PointVectorVector (vector<vector<Point>>) - zero-copy, native C++ type
  - Traditional JS array - backward compatible, converts to Contour objects
- ✓ Added PointVector support to contourArea function
- ✓ All three output types produce identical results (tested with 2 rectangles)
- ✓ Comprehensive test suite: test_findcontours_vectors.js

**Testing Results:**
- MatVector: 2 contours found, areas 3481 and 2401 ✓
- PointVectorVector: 2 contours found, areas 3481 and 2401 ✓
- Traditional array: 2 contours found, areas 3481 and 2401 ✓
- All areas match across all three output types ✓
- Hierarchy support working ✓

**Performance Benefits:**
- MatVector: Zero-copy, direct access to OpenCV's internal Mat objects
- PointVectorVector: Zero-copy, uses native C++ vector<vector<Point>> type
- Traditional array: Requires conversion (backward compatibility)

**See test_findcontours_vectors.js for usage examples.**

**Process:**
1. Identify all functions using Contour parameters
2. Check if they use generic array wrappers or Contour-specific types
3. Test with MatVector where possible
4. Document which functions need work vs. which already work

### Binary Compatibility Insight ("Husarenstück")
**Key insight:** Point and Vec2i have identical memory layout (8 bytes).

**Opportunities:**
- PointIterator can work with Mat CV_32SC2 data (reinterpret as Vec2i*)
- LineIterator can work with Vec4i data (HoughLinesP output)
- Zero-copy iteration on Mat-based contours
- See BUGS: `make-dormant-point-line-iterator-plug-into-point-mat-vector`

**Not a full solution:** Enables efficient iteration but doesn't solve all API compatibility issues.

### Rationale for Phased Approach
1. **Method migration is fastest path to opencv.js compatibility** - existing opencv.js code can work immediately
2. **MatVector is complex** - requires changes to findContours, all contour functions, client code
3. **100% MatVector migration unlikely** - some functions may need Contour internally for performance or type safety
4. **Individual assessment required** - can't assume all functions work the same way
5. **Test coverage critical** - verify each function works with new API

### Affected Code
- **C++:** js_contour.cpp/hpp, js_imgproc.cpp (findContours), js_cv.cpp (shape analysis), js_mat.cpp, js_draw.cpp, js_umat.hpp, js_point_iterator.cpp
- **JS:** 18+ files in qjs-opencv/tests, qjs-opencv, and plot-cv (see BUGS for full list)

### Testing Infrastructure
- **Framework:** tinytest.js (copied from ../quickjs/qjs-modules/tests/)
- **Location:** tests/unittests/
- **Pattern:** One test file per functionality group (test_contour_functions.js, test_matvector.js, etc.)
- **Coverage:** Verify freestanding functions work, then MatVector integration

### Success Criteria
1. **Phase 1:** All Contour methods available as freestanding functions, tests pass
2. **Phase 2:** MatVector class implemented, findContours outputs MatVector
3. **Phase 3:** Each contour function assessed and documented for MatVector compatibility
4. **Overall:** Existing opencv.js code can run on qjs-opencv with minimal changes

---

## Tier 1 — bind next

Small, self-contained additions to files that already exist. Each one either completes a pipeline stage that's currently a dead end, or gives Canny→findContours a materially better input image.

- [x] **photo module** (was 0/30 bound) → `js_photo.cpp`, wired into `init_module.cpp`, `tests/test_photo.js` demonstrates all 7 functions.
  - `pencilSketch`, `stylization`, `detailEnhance`, `edgePreservingFilter` — non-photorealistic-rendering filters that convert a photo *directly* into line art; worth an A/B test against the current Canny+findContours approximation.
  - `fastNlMeansDenoising`/`fastNlMeansDenoisingColored` — strips webcam sensor noise that currently becomes spurious tiny contours in the SVG output.
  - `inpaint` — removes dust/scratches from scanned source images before vectorizing.
  - **Gotcha found & guarded:** `cv.imread()` can return a 4-channel (BGRA) Mat for PNGs with alpha (this project's custom PNG reader preserves alpha; plain `cv::imread` would not). `pencilSketch`/`stylization`/`detailEnhance`/`edgePreservingFilter`/`fastNlMeansDenoisingColored` assume `CV_8UC3` with no validation of their own in this OpenCV build — a 4-channel input doesn't throw, it corrupts the heap (nondeterministic glibc `malloc` aborts, sometimes several calls later). Added an explicit channel-count check (`js_photo_require_channels`) that throws a clean `TypeError` instead; `inpaint` similarly checks for 1-or-3 channels. This was a real boundary-validation case, not speculative — first reproduction was exactly this PNG-with-alpha path.
  - Build-hygiene note for next time: this repo's `build/x86_64-linux-gnu` had stale object files compiled with a different `USE_FEATURE2D`/`USE_BARCODE`/`CXX2A` flag set than current `compile_commands.json` (and `cmake .` reconfigure currently fails here with a `check_library_exists`/CXX-feature-detection error, unrelated to this change). A full recompile of all 55 files from `compile_commands.json` fixed a `js_feature2d.so` linking against an effectively-empty stale object. Worth a clean `rm -rf build/x86_64-linux-gnu && cfg ...` at some point rather than continuing to patch around it.

- [ ] **calib3d — finish the calibration round-trip** → extend `js_calib3d.cpp` (86/106 bound)
  - `undistort`, `initUndistortRectifyMap`, `getOptimalNewCameraMatrix` — `calibrateCamera` is bound but nothing currently *applies* the resulting camera matrix/distortion coefficients; lens-distorted webcam/wide-angle rigs feed skewed contours straight into the SVG today.
  - `Rodrigues`, `solvePnP` — pose-from-known-points, useful for a fixed calibration jig on the cutting bed.

- [ ] **ximgproc — edge-aware smoothing filters** → extend `js_ximgproc.cpp` (55/89 bound)
  - `guidedFilter`, `dtFilter`, `l0Smooth`, `jointBilateralFilter`, `bilateralTextureFilter` — smooth flat regions while keeping strong edges crisp; run before Canny/findContours to cut noise contours in gradients/textures (skin, wood grain, fabric).
  - `fastBilateralSolverFilter` — upsamples a coarse/noisy edge or mask to full resolution snapped to real edges; useful if a reduced-resolution DNN edge pass (see Tier 2) is added later.

- [ ] **imgproc / draw — parity gaps** → extend `js_imgproc.cpp`, `js_draw.cpp` (97/202 bound)
  - `Laplacian`, `Scharr` — standard second-derivative edge operators missing next to the already-bound `Sobel`.
  - `approxPolyN` — newer fixed-vertex-count replacement for `approxPolyDP`; useful for clean N-gon simplification of a contour for laser paths.
  - `arrowedLine` — one-line addition to the draw module for debugging pipeline direction/normals.

- [ ] **xphoto — oil-painting stylization, stronger denoise** → extend `js_white_balancer.cpp` (or split into `js_xphoto.cpp`) (7/11 bound)
  - `oilPainting` — alternative stylization look distinct from `pencilSketch`.
  - `bm3dDenoising` — stronger (slower) denoiser for genuinely noisy low-light frames.

## Tier 2 — bind if the use case shows up

Real capability gains, but bigger binding surfaces or contingent on a workflow (live video, oversized stock, learned edge models) that isn't confirmed yet.

- [ ] **video — optical flow & frame stabilization** → new `js_video.cpp` (22/24 bound)
  - `calcOpticalFlowPyrLK`, `calcOpticalFlowFarneback`, `findTransformECC` — frame alignment for a jittery handheld/clamped webcam feed before averaging or temporal denoising.
  - `cv::KalmanFilter` — smooth a tracked contour/point across frames instead of re-detecting from scratch.
  - Only worth it once there's an actual live-capture workflow to stabilize.

- [ ] **features2d — keypoint detectors & matchers** → extend `js_feature2d.cpp` (1/64 bound)
  - `ORB::create`, `AKAZE::create`, `BRISK::create`, `SIFT::create` + `BFMatcher`/`FlannBasedMatcher`.
  - Note: these are only reachable via static `::create()` factories in modern OpenCV (no public constructor) — that's why they show as unbound classes in the raw coverage report even though the functionality exists as unbound factory functions.
  - Enables keypoint-based frame registration: aligning multiple shots of an oversized workpiece, or de-jittering a handheld capture by homography instead of dense optical flow.

- [ ] **stitching — `Stitcher` facade for oversized stock** → new `js_stitching.cpp` (facade only, not the full module) (0/221 bound)
  - Bind just `cv::Stitcher::create()`/`::stitch()`; almost all of the 221 unbound functions are internal (warpers, bundle adjusters, seam finders) that don't need direct exposure.
  - Payoff if the cutting bed or workpiece is larger than one camera frame: photograph in overlapping tiles, stitch, then run the existing contour pipeline on the composite.
  - Skip unless that's an actual scenario.

- [ ] **objdetect — QR/fiducial markers, cascade detection** → new `js_objdetect.cpp` (38/41 bound)
  - `QRCodeDetector`/`GraphicalCodeDetector` — printed fiducial marker for bed registration or scale calibration (derive a pixel-to-mm homography automatically from a known QR in-frame).
  - `CascadeClassifier` — lower priority, only if there's a concrete "find this specific object in frame" need.
  - **Check first:** the repo already ships `js_barcode_detector.cpp`, but `cv::barcode::BarcodeDetector` shows up as *unbound* in this scan — verify whether `USE_BARCODE` was off for this debug build, or whether the symbol moved when OpenCV merged barcode into `objdetect`. May be a build-config fix rather than new binding work.

- [ ] **dnn — high-level Model wrappers for learned edge detection** → extend `js_dnn.cpp` (139/172 bound)
  - `DetectionModel`/`SegmentationModel`/`ClassificationModel` — ergonomic pre/post-processing wrappers around the already-bound raw `Net`.
  - Payoff: a pretrained edge-detection network (HED, DexiNed) via `readNet` + a thin wrapper produces cleaner, more semantically-aware line art than Canny on complex photographs.
  - Needs shipping/downloading a model file — why this is Tier 2 not Tier 1.

## Tier 3 — skip for this project

No clear path back to "video source → SVG for a laser cutter." Listed so the reasoning is on record.

- **rgbd, sfm, stereo, structured_light, surface_matching** — 3D reconstruction; output is depth/point clouds, not 2D vector paths.
- **gapi** — declarative graph-compute engine, not a vision algorithm; its 487 unbound functions are the largest raw count in the whole report but zero relevance.
- **ml** — generic classifiers/regressors, no concrete use case in this pipeline yet.
- **text** — OCR, only relevant if vectorizing scanned document text specifically.
- **tracking** — legacy single-object trackers (KCF/CSRT/MIL); the Tier 2 video candidate covers the real motion need with less surface area.
- **optflow** — exotic dense-flow variants (DIS, DeepFlow) beyond what Farneback/PyrLK in Tier 2 already cover.
- **face** — face landmarks/recognition, out of scope.
- **xfeatures2d** — SURF and other patent-encumbered/legacy descriptors; features2d (Tier 2) already covers the modern equivalents.
- **saliency, quality, img_hash** — frame selection/scoring; nice-to-have, no current pipeline stage needs it.
- **line_descriptor** — line-segment matching across frames; redundant with the already-bound LineSegmentDetector/FastLineDetector.
- **videostab** — full video stabilization pipeline; Tier 2's ECC/optical-flow primitives cover the useful subset directly.
- **datasets, flann, reg, shape, fuzzy, hfs, hdf, dpm, mcc, rapid, wechat_qrcode, xobjdetect, bioinspired, alphamat, intensity_transform, phase_unwrapping, signal, plot, dnn_objdetect, dnn_superres, ccalib, cvv** — niche, deprecated, or infrastructure-only; no identified use case.

## API discrepancies vs opencv.js (shared surface only)

**IMPORTANT: When fixing opencv.js compatibility issues, always update all client scripts.** After changing a C++ binding to match opencv.js behavior, you MUST update every JavaScript file that uses the affected function:
```bash
grep -r -l "^import.*'opencv" examples/ tests/ *.js ../*.js
```
Update the call sites to use the opencv.js-compatible form, even if qjs-opencv retains backward compatibility (e.g., supporting both 1-arg and 2-arg signatures). The goal is to make client scripts as opencv.js-portable as possible.

Investigated 2026-08-17 (previous pass: 2026-08-12) by comparing `doc/opencv-js-api.md` (the official opencv.js binding surface, compiled from OpenCV 5.0.0's `platforms/js/opencv_js.config.py` + `modules/js/src/core_bindings.cpp` + `helpers.js`) against this project's own bindings, function-by-function, for names that exist **on both sides**. This is not a coverage comparison — qjs-opencv deliberately binds far more of `cv::` than opencv.js's hand-curated browser whitelist (see Tiers above). It's a divergence audit: where the same-named call exists in both, does it behave the same way? Anyone porting opencv.js snippets/tutorials into this project's `qjs` REPL will hit these.

The codebase moved substantially since the 2026-08-12 pass: `js_contour.[ch]pp` (the custom `Contour` class), `js_point_iterator`, `js_line_iterator`, and `js_slice_iterator` were removed entirely; `findContours` now natively supports `MatVector`/`PointVectorVector` output; `drawContours` (plural) now threads a real `index`/`hierarchy` through instead of hardcoding `index=0`; `cv.Range` is now a real constructible class; `Mat.diag()`, the seven `.dataXX` typed views, the seven `<type>At()` accessors, and the seven `<type>Ptr()` accessors all shipped; `Scalar.all()` and `barcode_BarcodeDetector` both shipped. Several other entries from the old pass are unchanged — re-verified below, not just carried forward blindly. Entries marked "carried forward, not re-verified" cover files with no commits since 2026-08-12 — low risk of drift, flagged transparently rather than silently repeated as fresh findings.

Grouped by how badly a naive port breaks.

### Silently wrong results (no exception, no obviously-missing output — the dangerous category)

- **`Mat.mul(otherMat, scale)` does matrix multiplication, not elementwise product.** *(unchanged — still live)* opencv.js's `mat.mul(other, scale)` wraps `cv::Mat::mul()` (elementwise/Hadamard product). This project's `.mul()` (`js_mat.cpp:831`, `MAT_EXPR_MUL` in `js_mat_expr`) executes `cv::Mat::operator*` (real matrix multiplication) whenever the argument is another Mat, and silently drops the `scale` argument in that branch (`scale` only applies in the scalar-operand branch at line 793). Two same-size square Mats will "work" and produce silently wrong numeric output; non-square/mismatched-inner-dimension Mats throw a `cv::Exception` where opencv.js would have succeeded.
- **`estimateAffine2D`/`estimateAffinePartial2D` silently discard the `inliers` output argument.** *(unchanged — still live)* Both hardcode `JSOutputArray inliers = cv::noArray()` (`js_calib3d.cpp:112`, `:174`) and never read `argv[2]`, jumping straight from arg 1 to arg 3 (`method`). Passing an output Mat at position 2 (opencv.js's convention) is silently ignored — no error, the inlier mask is just never populated.
- **`floodFill`'s `rect` output argument is never written back.** *(unchanged — still live)* `js_imgproc.cpp:1221-1240` (`MISC_FLOOD_FILL`): `rectPtr` receives the flood-filled bounding box from the native call but is never copied back onto the caller's `rect` object afterward — opencv.js mutates the passed-in object's `x`/`y`/`width`/`height` in place. Also: this project's `floodFill` still has no `mask` parameter at all (built on the mask-less overload), where opencv.js's directly-bound version is `floodFill(img, mask, seedPoint, newVal, rectOut, loDiff, upDiff, connectivity)`.
- **`Feature2D.compute()` doesn't copy the (possibly filtered) keypoints back to the caller.** *(unchanged — still live)* `js_feature2d.cpp:987-989`: the copy-back line is present but still commented out. Descriptor extractors that drop keypoints they can't compute a descriptor for leave the caller's JS array stale. `detect()` (line 1003) does copy back correctly; `compute()` still doesn't.
- **`moments(points, binaryImage)` uses `binaryImage` to choose the input's *interpretation*, not just pixel binarization.** *(unchanged — still live)* `js_imgproc.cpp:1288-1302`: `binaryImage === false` unconditionally parses `argv[0]` as a polygon point array. Real `cv::moments`/opencv.js decide points-vs-raster from the actual `InputArray` content; passing a grayscale `Mat` with `binaryImage=false` (valid opencv.js usage, for intensity-weighted image moments) does not work here.
- **`Mat.step` returns `dims - 1` entries, not `dims`.** *(unchanged — still live)* `js_mat.cpp:1465` loops `i < m->dims - 1`, silently dropping the last dimension's stride. opencv.js's `.step` (`getMatStep`) always returns one entry per dimension.

*(Removed from this list, now fixed: `HuMoments` silently dropping results for non-Array outputs — `js_imgproc.cpp:2220-2233` (`SHAPE_HU_MOMENTS`) now routes the 2-arg form through `js_cv_outputarray()`, a generic `OutputArray` accepting `Mat` and matching opencv.js's convention, rather than gating on `JS_IsArray`.)*

### Not bound at all (opencv.js has them, qjs-opencv doesn't)

- **`CamShift`/`meanShift`** — *(unchanged — still live)* both are registered in opencv.js's `core_bindings.cpp` (returning `[RotatedRect, updatedRect]` and `[n, updatedRect]` arrays respectively). Still no bindings for either anywhere in the tree; only the unrelated `pyrMeanShiftFiltering` exists. Ported tracking code calling `cv.CamShift(...)` or `cv.meanShift(...)` will throw `TypeError`.
- **`cv.createCLAHE(clipLimit, tileGridSize)` free function doesn't exist.** *(unchanged — still live)* `js_clahe.cpp:28` calls `cv::createCLAHE()` internally inside the `new CLAHE(...)` constructor, but never exports a module-level `createCLAHE` function — opencv.js exposes *only* the free-function form, no `new cv.CLAHE(...)` at all. Ported code calling `cv.createCLAHE(...)` throws `TypeError: cv.createCLAHE is not a function`.
- **`ORB`/`MSER`/`AKAZE`/`BRISK`/`FastFeatureDetector`/`GFTTDetector` have no `.create()` static.** *(unchanged — still live)* `js_feature2d.cpp:1224-1233` registers all of these as `new`-constructible classes only (their static-func tables hold only enum constants, e.g. `ORB.HARRIS_SCORE`/`ORB.FAST_SCORE` — no `create`). opencv.js's whitelist only exposes `XXX.create(...)`, no public constructor, for all of these.
- **`StringVector`, `DMatchVectorVector`, `KeyPointVectorVector`** (all present in opencv.js's vector-type whitelist per `doc/opencv-js-api.md`) — not found anywhere in `js_vector.cpp`'s dispatch or as separate `.hpp` files; still unbound.

*(`Mat.diag()`, `Mat.data`/`.data8S`/`.data16U`/`.data16S`/`.data32S`/`.data32F`/`.data64F`, all seven `<type>At()` accessors, and all seven `<type>Ptr()` accessors from the 2026-08-12 audit's "not bound" list are now **implemented** — `js_mat.cpp:2051-2101` — and removed from this list.)*

### Different return shape / calling convention (throws or misbehaves on an opencv.js-style call, but obviously so)

- **`minMaxLoc` returns a positional array, not a named object.** *(unchanged — still live)* `js_cv.cpp:1157-1179` (`OTHER_MIN_MAX_LOC`): opencv.js returns `{minVal, maxVal, minLoc, maxLoc}`. This project returns `[minVal, maxVal, minLoc, maxLoc]` — `.minVal` etc. are `undefined`; must destructure by index.
- **`minEnclosingCircle` has no return value at all.** *(carried forward, not re-verified)* opencv.js returns `Circle {center, radius}`; this project reports the result only through optional callback arguments, with no `Circle` type anywhere in the codebase.
- **`kmeans`'s `criteria` argument shape differs, and no `TermCriteria` class exists anywhere.** *(unchanged — still live)* `js_cv.cpp:1068-1092`: `criteria` is read via `js_array_to(ctx, argv[3], crit)` into a plain `std::vector<double>` (needs `crit.size() >= 3`), not opencv.js's `TermCriteria {type, maxCount, epsilon}` object. Grepped the whole tree for a `TermCriteria` JS class — none exists; every internal use (`js_fisheye.cpp`, `js_calib3d.cpp`, `js_cv.cpp`) hardcodes its own C++-side `cv::TermCriteria`, none of it constructible/settable from JS.
- **`new Mat(otherMat)` (the copy-handle constructor) still doesn't exist — throws.** *(unchanged — still live)* `js_mat_initialize`'s full dispatch chain (`js_mat.cpp:342-430`+): array / typed-array / size / rows,cols / vector branches all present, no branch recognizing another `Mat` as `argv[0]`. opencv.js's `new cv.Mat(other)` creates a shallow-copy handle; the equivalent here still only exists as the `.dup()` instance method.
- **`Mat.empty`/`.continuous` are JS accessor properties, not callable methods.** *(unchanged — still live)* `js_mat.cpp:2044,2046` (`JS_CGETSET_MAGIC_DEF`) — `mat.empty()`/`mat.continuous()` both throw (`TypeError: not a function`); must read `mat.empty`/`mat.continuous` with no parens. `.isContinuous()` (the opencv.js name) doesn't exist as a property or method name at all — only the renamed `.continuous` getter does.
- **No `cv.Scalar` constructible type, and it doesn't subclass `Array`.** *(partially fixed since 2026-08-12)* `Scalar.all(v)` now exists (`js_cv.cpp:1440`, `:2605-2606`) and returns a uniform fill, matching opencv.js. But there's still no `new cv.Scalar(...)` constructor — only the `js_scalar_read`/`js_scalar_new` helpers operating on plain arrays/`Float64Array` — and the return type is a `Float64Array`, not an `Array` subclass (`scalar instanceof Array === false` here, `true` in opencv.js).
- **`RotatedRect.points`/`.boundingRect`/`.boundingRect2f` are instance methods here, static functions in opencv.js.** *(unchanged — still live)* `js_rotated_rect.cpp:255`: `rr.points()`, not opencv.js's `RotatedRect.points(rr)`.
- **`DescriptorMatcher`'s static function table is still empty (no `.create()`), `BFMatcher` is still `new`-constructible.** *(carried forward, not re-verified)* opencv.js relies on `DescriptorMatcher.create(type)`/`BFMatcher.create(normType, crossCheck)`.
- **`SimpleBlobDetector`'s constructor still ignores/has no way to take a `Params` argument.** *(carried forward, not re-verified)*
- **`cv.drawKeypoints`'s `flags` parameter is hardcoded, `color`'s optionality check is still broken.** *(unchanged — still live)* `js_draw.cpp:483-498`: `flags` is always `cv::DrawMatchesFlags(0)`. The `if(argc)` guard meant to detect an omitted `color` argument is always true (the function's declared minimum arity is already 3, so `argc >= 3` unconditionally), so it always reads `argv[3]` rather than falling back to the default (already correctly matching opencv.js's `Scalar.all(-1)` random-color convention *when actually used*) whenever `color` is omitted.
- **`calibrateCameraExtended` doesn't exist as a separate name — folded into `calibrateCamera`.** *(carried forward, not re-verified)*
- **`findHomography`'s point inputs go through a custom point-array reader, not a generic `InputArray`.** *(carried forward, not re-verified)*

*(Removed from this list, now fixed: `BarcodeDetector` naming — `js_barcode_detector.cpp:272-273,296-297` now exports both bare `BarcodeDetector` **and** `barcode_BarcodeDetector`, matching opencv.js's namespace-flattened name. `drawContours` with `index >= 0` breaking hierarchy-aware drawing — `js_draw_contours` (plural, `js_draw.cpp:220-264`) now threads the real `index`/`hier`/`maxLevel` straight into `cv::drawContours` against the full `contours` array/vector, no single-contour re-wrap. "No `cv.Range` class, array-shape-only" — `js_range.cpp`/`js_range.hpp` now provide a real constructible `cv.Range` class whose reader (`js_range_read`, `js_range.hpp:42-69`) accepts both the `[start,end]` array form and opencv.js's `{start,end}` object form.)*

### Vector container types — now bound, with remaining gaps

- **`MatVector`/`PointVector`/`Point2fVector`/`Point3fVector`/`KeyPointVector`/`DMatchVector`/`RectVector`/`IntVector`/`FloatVector`/`DoubleVector`/`CharVector`/`PointVectorVector`/`CharVectorVector` all now exist**, via the generic `JSVector<T>` template (`include/js_vector.hpp`) + one `.cpp`/`.hpp` per type. Each has `.size()`, `.get(i)`, `.push_back(v)`, `.set(i,v)`, `Symbol.iterator`, **and** a `.delete()` method (`js_vector.hpp:194-204`) that's safe to call even though this project's own memory model doesn't need it — good, since ported opencv.js code calling `.delete()` explicitly won't throw.
- **`findContours` now accepts `MatVector` or `PointVectorVector` as the `contours` output**, zero-copy, matching opencv.js's convention (`js_imgproc.cpp:964-1010`, `js_cv_find_contours`). A plain JS `Array` is still also accepted as a backward-compat fallback (populated with `Mat` CV_32SC2 objects — the old custom `Contour` type no longer exists at all).
- **Still not migrated to `MatVector`/vector-of-vectors conventions:** `split`/`merge` — presumed still plain JS `Array` of `Mat`s (no commits touched these call sites since 2026-08-12; not re-verified this pass). `HoughLines` (but inconsistently *not* `HoughLinesP`/`HoughCircles`) — presumed still requires a plain JS Array for `lines` (not re-verified this pass).

### Different defaults / optionality (same call shape, different silent behavior)

- **Default line type**: `circle`/`ellipse`/`line`/`putText` default to `cv::LINE_AA` (antialiased) in this project; native OpenCV/opencv.js default to `cv::LINE_8`. *(carried forward, not re-verified)*
- **Default thickness for `circle`/`ellipse`**: defaults to `-1` (filled) here vs `1` (1px outline) in opencv.js/native OpenCV. *(carried forward, not re-verified)*
- **`circle`/`ellipse` have no `shift` parameter**. *(carried forward, not re-verified)*
- **`fillPoly` isn't independently callable** — merged with `polylines` into one dispatcher. *(carried forward, not re-verified)*
- **`aruco.drawDetectedMarkers`'s default `borderColor`** is `{0,0,255}` (red-ish BGR) here vs opencv.js's `Scalar(0,255,0)` (green). *(re-verified this pass — `js_aruco.cpp:46,63` — still live)*
- **`inpaint`'s `flags` argument is optional here** (defaults to `INPAINT_TELEA`), **required with no default in opencv.js.** *(re-verified this pass — `js_photo.cpp:180` — still live)* This project also adds a channel-count validation `TypeError` opencv.js doesn't have.
- **`dnn.Net.setPreferableBackend` declares arity 0** even though `backendId` is effectively required. *(carried forward, not re-verified — no `js_dnn.cpp` commits since 2026-08-12)*
- **`readNet`/`readNetFromTensorflow`/`readNetFromTFLite` don't expose OpenCV 5's `engine` parameter**. *(carried forward, not re-verified — no `js_dnn.cpp` commits since 2026-08-12)*
- **`normalize` has no `mask` parameter**. *(carried forward, not re-verified)*
- **`connectedComponents`/`connectedComponentsWithStats` require an extra mandatory `ccltype` argument with no default**. *(carried forward, not re-verified)*
- **`mixChannels` accepts extra optional interleaved "count" arguments**. *(carried forward, not re-verified)*
- **`DescriptorMatcher.match()` dispatches on `argc` rather than argument type**. *(carried forward, not re-verified)*

---

## Appendix — full per-module unbound counts (cv:: namespace only)

`detail::`/`cuda::`/`ocl::`/`hal::` internals and modules with zero unbound `cv::` symbols are excluded.

| module | classes | functions | tier |
|---|---:|---:|---|
| core | 48 / 58 | 608 / 753 | internals |
| gapi | 57 / 57 | 487 / 487 | skip |
| stitching | 8 / 8 | 213 / 213 | tier 2 |
| dnn | 13 / 15 | 139 / 172 | tier 2 |
| imgproc | 3 / 5 | 97 / 202 | tier 1 |
| calib3d | 3 / 3 | 86 / 106 | tier 1 |
| features2d | 8 / 8 | 63 / 64 | tier 2 |
| optflow | 2 / 2 | 64 / 64 | skip |
| tracking | 29 / 29 | 34 / 34 | skip |
| objdetect | 19 / 19 | 38 / 41 | tier 2 |
| ximgproc | 1 / 1 | 55 / 89 | tier 1 |
| xfeatures2d | 2 / 2 | 53 / 53 | skip |
| rgbd | 12 / 12 | 40 / 40 | skip |
| videostab | 15 / 15 | 34 / 34 | skip |
| face | 7 / 7 | 40 / 40 | skip |
| text | 4 / 4 | 41 / 41 | skip |
| video | 21 / 21 | 22 / 24 | tier 2 |
| datasets | 0 / 0 | 37 / 37 | skip |
| ml | 4 / 4 | 29 / 29 | skip |
| sfm | 0 / 0 | 32 / 32 | skip |
| photo | 0 / 0 | 30 / 30 | tier 1 |
| img_hash | 1 / 1 | 25 / 25 | skip |
| surface_matching | 1 / 1 | 25 / 25 | skip |
| imgcodecs | 4 / 4 | 16 / 21 | low priority |
| superres | 1 / 1 | 19 / 19 | skip |
| videoio | 2 / 4 | 16 / 16 | low priority |
| aruco | 1 / 1 | 15 / 15 | low priority |
| fuzzy | 0 / 0 | 16 / 16 | skip |
| highgui | 0 / 0 | 16 / 41 | low priority |
| line_descriptor | 7 / 7 | 8 / 8 | skip |
| ccalib | 4 / 4 | 9 / 9 | skip |
| quality | 3 / 3 | 9 / 9 | skip |
| saliency | 4 / 4 | 8 / 8 | skip |
| flann | 11 / 11 | 0 / 0 | skip |
| xphoto | 0 / 0 | 7 / 11 | tier 1 |
| bgsegm | 1 / 1 | 4 / 9 | low priority |
