# TODO — binding backlog

Prioritized by leverage for plot-cv's actual pipelines — laser-cutting SVG export, and three planned vectorization examples (`examples/sticker2svg.js`, `examples/collage-art.js`, `examples/vector-trace.js`) — not by raw OpenCV API surface. Derived from a `scripts/binding_coverage.js` run against `build/x86_64-linux-debug/opencv.so` (OpenCV 4.13.0), filtered to `cv::`-namespace symbols only.

Regenerate the underlying data with:
```bash
qjsm scripts/binding_coverage.js --module=build/x86_64-linux-debug/opencv.so \
  --lib-dir=/opt/opencv-4.13.0-x86_64/lib --namespace=cv --verbose --out=cov.txt
```

## Already solved, don't rebuild

- **Contour → SVG bezier splines.** No OpenCV algorithm does this (checked the full coverage survey — no such symbol exists, bound or unbound). It doesn't need a library either: `js/cvVectorization.js` (uncommitted) already has a correct Schneider/Graphics-Gems `FitCurves` cubic-bezier fitter (`CurveFitter` — corner detection, chord-length parameterization, Newton-Raphson reparameterization, recursive error-based subdivision) consuming `contour.array` directly, plus `SvgBuilder` for multi-region path output with holes via `fill-rule="evenodd"`. Keep this in JS; it's O(n) per subdivision and QuickJS handles it fine. Only reconsider a C++ port if profiling on a real workload shows it's the bottleneck.
- **Polyline simplification.** `cv.Contour` already exposes all seven `psimpl` algorithms as methods (`simplifyDouglasPeucker`, `simplifyReumannWitkam`, `simplifyOpheim`, `simplifyLang`, `simplifyNthPoint`, `simplifyRadialDistance`, `simplifyPerpendicularDistance`) — see `js_contour.cpp`.
- **Skeleton tracing.** `algorithms/skeleton_lines.hpp` (Guo-Hall thinning + topology-aware tracing that cuts at junctions) is fully bound: `skeletonizeGuohall`, `traceLines`, `degreeMap`, `skeletonizeAndTrace`. Distinct from `findContours` (region boundaries) and `LineSegmentDetector`/`FastLineDetector` (straight-line detection) — the three don't substitute for each other, pick per input character.
- **Region proposal for collage-art's "several algorithms to pick a motive."** `grabCut` (bound) plus `ximgproc::segmentation` selective-search/graph-segmentation (already bound) cover this with zero new binding work.

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

Investigated 2026-08-12 by comparing `doc/opencv-js-api.md` (the official opencv.js binding surface, compiled from OpenCV 5.0.0's `platforms/js/opencv_js.config.py` + `modules/js/src/core_bindings.cpp` + `helpers.js`) against this project's own bindings, function-by-function, for names that exist **on both sides**. This is not a coverage comparison — qjs-opencv deliberately binds far more of `cv::` than opencv.js's hand-curated browser whitelist (see Tiers above). It's a divergence audit: where the same-named call exists in both, does it behave the same way? Anyone porting opencv.js snippets/tutorials into this project's `qjs` REPL will hit these.

Grouped by how badly a naive port breaks.

### Silently wrong results (no exception, no obviously-missing output — the dangerous category)

- **`Mat.mul(otherMat, scale)` does matrix multiplication, not elementwise product.** opencv.js's `mat.mul(other, scale)` wraps `cv::Mat::mul()` (elementwise/Hadamard product). This project's `.mul()` (`js_mat.cpp` `js_mat_expr`, `MAT_EXPR_MUL`) executes `cv::Mat::operator*` (real matrix multiplication) whenever the argument is another Mat, and silently drops the `scale` argument in that branch (`scale` only applies in the scalar-operand branch). Two same-size square Mats will "work" and produce silently wrong numeric output; non-square/mismatched-inner-dimension Mats throw a `cv::Exception` where opencv.js would have succeeded.
- **`estimateAffine2D`/`estimateAffinePartial2D` silently discard the `inliers` output argument.** Both hardcode `JSOutputArray inliers = cv::noArray()` (`js_calib3d.cpp`) and never read `argv[2]`, jumping straight from arg 1 to arg 3 (`method`). Passing an output Mat at position 2 (opencv.js's convention) is silently ignored — no error, the inlier mask is just never populated.
- **`floodFill`'s `rect` output argument is never written back.** The C++ call receives a `cv::Rect*` and computes the flood-filled bounding box, but the JS binding never copies the result back onto the caller's `rect` object afterward (`js_imgproc.cpp`) — opencv.js mutates the passed-in object's `x`/`y`/`width`/`height` in place. Also: this project's `floodFill` has no `mask` parameter at all (built on the mask-less overload), where opencv.js's directly-bound version is `floodFill(img, mask, seedPoint, newVal, rectOut, loDiff, upDiff, connectivity)`.
- **`HuMoments` silently drops results if the output isn't a plain JS Array.** Only fills the output if `JS_IsArray(argv[1])` passes; passing a `Mat` (opencv.js's normal `OutputArray` convention) results in no error and no written values.
- **`Feature2D.compute()` doesn't copy the (possibly filtered) keypoints back to the caller.** The copy-back line is present but commented out (`js_feature2d.cpp`) — descriptor extractors that drop keypoints they can't compute a descriptor for leave the caller's JS array stale. `detect()` in the same file does copy back correctly; `compute()` doesn't.
- **`moments(points, binaryImage)` uses `binaryImage` to choose the input's *interpretation*, not just pixel binarization.** `binaryImage === false` unconditionally parses `argv[0]` as a polygon point array. Real `cv::moments`/opencv.js decide points-vs-raster from the actual `InputArray` content; passing a grayscale `Mat` with `binaryImage=false` (valid opencv.js usage, for intensity-weighted image moments) does not work here.
- **`drawContours` with `index >= 0` breaks hierarchy-aware drawing.** Extracts just that one contour into a fresh length-1 vector and always calls the native function with `index=0` — semantics diverge from opencv.js whenever `hierarchy`/`maxLevel` matter, since the full contour list is no longer intact for that call.
- **`Mat.step` returns `dims - 1` entries, not `dims`.** `js_mat.cpp` loops `i < m->dims - 1`, silently dropping the last dimension's stride. opencv.js's `.step` (`getMatStep`) always returns one entry per dimension.

### Different return shape / calling convention (throws or misbehaves on an opencv.js-style call, but obviously so)

- **`minMaxLoc` returns a positional array, not a named object.** opencv.js: `{minVal, maxVal, minLoc, maxLoc}`. This project: `[minVal, maxVal, minLoc, maxLoc]` — `.minVal` etc. are `undefined`; must destructure by index. Argument order for `src`/`mask` also differs (`mask` is arg 5 here, behind extra optional per-value callback args; arg 2 in opencv.js).
- **`minEnclosingCircle` has no return value at all.** opencv.js returns `Circle {center, radius}`. This project reports the result only through two optional callback arguments (`argv[1]`/`argv[2]`), and there is no `Circle` type anywhere in the codebase.
- **`kmeans`'s `criteria` argument shape differs.** opencv.js expects a `TermCriteria {type, maxCount, epsilon}` object. This project expects a plain 3-element array/array-like (`js_array_to`); a `TermCriteria`-shaped object passed positionally won't be read correctly (this project has no `TermCriteria` class at all — see below).
- **No `MatVector`/`PointVector`/`KeyPointVector`/`DMatchVector`/`RectVector` wrapper types exist.** opencv.js requires these explicit, manually-`.delete()`'d container classes for any function producing/consuming collections. This project uses plain JS arrays throughout (no `.delete()` needed — GC/finalizer-driven, consistent with this project's overall memory model), which affects the calling convention of every function below:
  - `split`/`merge` — plain JS `Array` of `Mat`s, not a `MatVector`.
  - `findContours` — `contours` out-param is a plain Array populated with this project's own `Contour` point-array type (not `Mat`s/`MatVector`); `hierarchy` polymorphically accepts a `Mat`, a plain Array, *or* a callback function depending on runtime type, none of which is opencv.js's single-Mat-out-param convention.
  - `HoughLines` (but inconsistently *not* `HoughLinesP`/`HoughCircles`, which do use Mat-typed output, matching opencv.js) — requires a plain JS Array for `lines`, mutated in place with `[rho, theta]` sub-arrays.
- **`new Mat(otherMat)` (the copy-handle constructor) doesn't exist — throws.** opencv.js's `new cv.Mat(other)` creates a shallow-copy handle. This project's constructor dispatch has no branch recognizing another `Mat` as `argv[0]`; the equivalent behavior exists only as the `.dup()` instance method.
- **`Mat.empty` is a property, not a method — `mat.empty()` throws.** Must use `mat.empty` (no parens). Contrast with `.channels`/`.type`/`.depth`/`.elemSize`/`.elemSize1`, which this project *does* make both callable and readable via a "number box" wrapper — `.empty` doesn't get that treatment.
- **`Mat.isContinuous()` doesn't exist — renamed to a plain boolean property `.continuous`.** `mat.continuous()` also throws (not callable).
- **`Mat.ptr(i[, j])` returns a typed array matching the Mat's own element depth, not always `Uint8Array`.** opencv.js's generic `.ptr()` always returns `Uint8Array` (separate `ucharPtr`/`floatPtr`/etc. exist for typed views); this project's single `.ptr()` infers the JS TypedArray type from `mat.type()`.
- **`Mat.rowRange(range)`/`.colRange(range)` single-argument `Range`-object form doesn't work.** Only the two-number `(start, end)` form is implemented; a `Range`-shaped argument gets coerced via `JS_ToInt64` on an object rather than read as a range.
- **No `cv.Scalar` constructible type.** opencv.js's `Scalar` is a real class (`new cv.Scalar(...)`, `Scalar.all(v)`, subclasses `Array`). This project only has loose `js_scalar_read`/`js_scalar_new` helpers operating on plain arrays — no constructor, no `.all()`.
- **No `cv.Range` class, and the array shape differs from opencv.js's object shape.** This project's `js_range_read` requires a 2-element array `[start, end]`; opencv.js's `Range` is `{start, end}`. Passing an opencv.js-style object here silently fails validation and falls back to a full-range default (`{INT_MIN, INT_MAX}`) rather than an error.
- **`RotatedRect.points`/`.boundingRect`/`.boundingRect2f` are instance methods here, static functions in opencv.js.** opencv.js: `RotatedRect.points(rr)`. This project: `rr.points()`. The static form (`RotatedRect.from`) exists as a stub but is unimplemented (always throws/returns unset).
- **`KeyPoint` is fully constructible here (`new cv.KeyPoint(...)`), value-object-only in opencv.js.** Also has an extra `.hash()` method with no opencv.js counterpart. Field shape itself (`angle`/`class_id`/`octave`/`pt`/`response`/`size`) matches exactly.
- **`BarcodeDetector` naming**: opencv.js exposes `cv.barcode_BarcodeDetector` (namespace-flattened); this project exposes a bare global `BarcodeDetector`.
- **`createBackgroundSubtractorMOG2` returns a type-erased `BackgroundSubtractor`, not a distinct `BackgroundSubtractorMOG2`.** opencv.js has both the factory function *and* a separately-constructible `BackgroundSubtractorMOG2` class with its own setters/getters. This project's factory returns the same generic opaque type regardless of algorithm (MOG2/KNN/MOG/GMG/CNT/GSOC/LSBP all share one class), exposing only base `apply`/`getBackgroundImage` — no MOG2-specific tuning methods, no `instanceof` distinction.
- **`ORB`/`MSER`/`FastFeatureDetector`/`GFTTDetector`/`SimpleBlobDetector` are constructed via `new cv.XXX(...)`, not opencv.js's `cv.XXX.create(...)` factory.** None of these classes define a `.create()` static method in this project at all — opencv.js's whitelist only exposes `create`, no public constructor.
- **`BFMatcher`/`DescriptorMatcher` — same constructor-vs-factory mismatch.** This project binds `BFMatcher` as `new`-constructible; `DescriptorMatcher`'s static function table is empty (no `.create()` at all), where opencv.js relies on `DescriptorMatcher.create(type)`/`BFMatcher.create(normType, crossCheck)`.
- **`SimpleBlobDetector`'s constructor ignores/has no way to take a `Params` argument.** opencv.js: `SimpleBlobDetector.create(params?)`. This project's factory takes zero arguments; a `simple_blob_params` field is declared but dead (never read, never passed to `create()`), and there's no `setParams`/`getParams` either.
- **`cv.drawKeypoints`'s `flags` parameter is hardcoded, `color`'s optionality check is broken.** `flags` is always `cv::DrawMatchesFlags(0)` — no way to request e.g. `DRAW_RICH_KEYPOINTS`. The `if(argc)` guard meant to detect an omitted `color` argument is effectively always true (argc is already ≥ the declared minimum), so it always attempts to read `argv[3]` as a color rather than falling back to opencv.js's default `Scalar.all(-1)` (random per-keypoint color) when omitted.
- **`CLAHE` construction is inverted relative to opencv.js.** opencv.js only exposes `cv.createCLAHE(clipLimit, tileGridSize)` — no `new cv.CLAHE(...)`. This project is the reverse: only `new CLAHE(...)`, no `createCLAHE` free function at all.
- **`CLAHE.getClipLimit`/`setClipLimit`/`getTilesGridSize`/`setTilesGridSize` are JS accessor properties here, not methods.** `clahe.clipLimit = x` / `clahe.tilesGridSize`, not `clahe.getClipLimit()`/`.setClipLimit(x)` as in opencv.js. `apply`/`collectGarbage` do match opencv.js's method-call convention.
- **`blobFromImage`/`blobFromImages` accept an extra in-place calling convention opencv.js doesn't have.** If the 2nd argument is a JS object, this project reinterprets it as an output blob rather than `scalefactor` (a number in opencv.js, which only ever returns a new `Mat`) — a caller porting `cv.blobFromImage(img, 1.0)` style code is fine, but any code passing an object in that position behaves differently than opencv.js would (opencv.js has no output-array overload for `blobFromImage` at all — only `_W`-exported, return-value form is bound).
- **`calibrateCameraExtended` doesn't exist as a separate name — folded into `calibrateCamera`.** opencv.js whitelists *only* the extended overload, under the name `calibrateCameraExtended` (plain `calibrateCamera` isn't bound in opencv.js at all). This project has a single `calibrateCamera` that argc/type-sniffs to decide extended-vs-plain behavior, and hardcodes `criteria` to 30 iterations (vs opencv.js's default of 500) with no way to override it from JS.
- **`findHomography`'s point inputs go through a custom point-array reader, not a generic `InputArray`.** Restricts this project's `findHomography` to plain point arrays; arbitrary `Mat`/`UMat` inputs (valid via opencv.js's `InputArray` binding) aren't accepted the same way.

### Different defaults / optionality (same call shape, different silent behavior)

- **Default line type**: `circle`/`ellipse`/`line`/`putText` default to `cv::LINE_AA` (antialiased) in this project; native OpenCV/opencv.js default to `cv::LINE_8`.
- **Default thickness for `circle`/`ellipse`**: defaults to `-1` (filled) here vs `1` (1px outline) in opencv.js/native OpenCV.
- **`circle`/`ellipse` have no `shift` parameter** — native/opencv.js's trailing `shift=0` argument isn't reachable at all here.
- **`fillPoly` isn't independently callable** — merged with `polylines` into one `drawPolygon`/`Draw.polygon` function that dispatches on `thickness <= 0`, rather than opencv.js's two independent functions with independent signatures.
- **`aruco.drawDetectedMarkers`'s default `borderColor`** is `{0,0,255}` (red-ish BGR) here vs opencv.js's `Scalar(0,255,0)` (green).
- **`inpaint`'s `flags` argument is optional here** (defaults to `INPAINT_TELEA`), **required with no default in opencv.js.** This project also adds a channel-count validation `TypeError` opencv.js doesn't have (opencv.js passes any channel count straight to the native call).
- **`dnn.Net.setPreferableBackend` declares arity 0** even though `backendId` is effectively required — omitting it silently passes `-1` (an invalid enum) to `cv::dnn::Net::setPreferableBackend` instead of erroring at the JS call-arity level.
- **`readNet`/`readNetFromTensorflow`/`readNetFromTFLite` don't expose OpenCV 5's `engine` parameter** (`ENGINE_AUTO`/`ENGINE_CLASSIC`/`ENGINE_ORT` selection) — opencv.js's signatures for all three include a trailing `engine` argument (plus `extraOutputs` for `readNetFromTensorflow`); this project always uses the C++ default. Inconsistent even within this project: `readNetFromONNX` *does* thread `engine` through. `readNet` also lacks opencv.js's buffer-based overload entirely (path-only).
- **`normalize` has no `mask` parameter** — opencv.js/native `cv::normalize` has a trailing `mask=noArray()` argument; this project's binding stops at `dtype`.
- **`connectedComponents`/`connectedComponentsWithStats` require an extra mandatory `ccltype` argument with no default**, and `connectivity`/`ltype` have no defaults either — opencv.js's plain (non-"WithAlgorithm") overloads default all three (`connectivity=8, ltype=CV_32S`, no `ccltype` in the public overload at all).
- **`mixChannels` accepts extra optional interleaved "count" arguments** after each vector to truncate them — not present in opencv.js's plain 3-argument `(src, dst, fromTo)` form.
- **`DescriptorMatcher.match()` dispatches on `argc` rather than argument type** (`argc > 3` selects the 4-arg train-descriptors overload) — a minor fragility relative to opencv.js's type-resolved overloads, not a hard bug, but worth knowing if a 3-arg call ever needs to pass `train` positionally.

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
