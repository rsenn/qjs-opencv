# TODO — binding backlog

Prioritized by leverage for plot-cv's actual pipelines — laser-cutting SVG export, and three planned vectorization examples (`examples/sticker2svg.js`, `examples/collage-art.js`, `examples/vector-trace.js`) — not by raw OpenCV API surface. Derived from a `scripts/binding_coverage.js` run against `build/x86_64-linux-debug/opencv.so` (OpenCV 4.13.0), filtered to `cv::`-namespace symbols only.

Regenerate the underlying data with:
```bash
qjsm scripts/binding_coverage.js --module=build/x86_64-linux-debug/opencv.so \
  --lib-dir=/opt/opencv-4.13.0-x86_64/lib --namespace=cv --verbose --out=cov.txt
```

## getScreenResolution() build-config fix (2026-09-08, root cause fixed 2026-09-09)

`js_highgui.cpp`'s `getScreenResolution()` has an `#if _WIN32 / #elif
defined(HAVE_X11)` gate, but this build's CMake `HAVE_LIBX11` check fails
even though X11 clearly works (namedWindow/imshow/etc. all work fine), so
`HAVE_X11` never gets set and neither branch compiles - `width`/`height`
were left uninitialized. Symptom escalated across a real session: first
observed as a GUI window opening too small (garbage happened to read as
`{0,0}` in that environment), then as a hard crash on another machine -
`cv::Exception ... Failed to allocate 34558810068822 bytes` - because
there the same uninitialized memory happened to read as a large positive
number, which `js/vectorizer/gui/app.js`'s first-pass fix (`> 0` check
only) let through straight into `new Canvas(w, h)` → `new Mat(...)`.

Fixed *for now* at both layers (both left in place as defense in depth):
- `js_highgui.cpp`: zero-initialize `width`/`height`, so a build where
  neither branch compiles deterministically returns `{0,0}` instead of
  random stack garbage.
- `js/vectorizer/gui/app.js`: check the reported size against a plausible
  real-monitor range (320..16384 per axis), not just `> 0`, before trusting
  it - falls back to a 1920x1080 guess otherwise.

**Root cause found and fixed (2026-09-09):** it was never a detection
failure - `CMakeCache.txt` already had `HAVE_LIBX11:INTERNAL=1` and
`HAVE_X11_X_H:INTERNAL=TRUE`, and `CMakeConfigureLog.yaml` shows the
`check_library_exists(X11 XOpenDisplay ...)` try-compile succeeding
(`exitCode: 0`). The actual bug: `CMakeLists.txt:99-101` computed
`HAVE_X11` via a plain `set(HAVE_X11 TRUE)` but never forwarded it to the
compiler as a `-D` flag, unlike every other `HAVE_*` flag in that file
(all go through `add_definitions(-D...)` or `functions.cmake`'s
`var2define`). `HAVE_X11` was true in CMake's own variable scope the whole
time; the compiler just never saw it. Fixed by adding
`add_definitions(-DHAVE_X11)` inside the `if(HAVE_LIBX11 AND
HAVE_X11_X_H)` block. Rebuilt clean, confirmed `-DHAVE_X11` now appears in
`build/x86_64-linux-gnu/CMakeFiles/quickjs-opencv.dir/flags.make`, and
`cv.getScreenResolution()` now returns the real display size (1920x1080)
instead of the `{0,0}` fallback. No explicit `-lX11` link line was needed -
the symbols (`XOpenDisplay`, `DefaultScreenOfDisplay`) resolve fine at
module-load time since OpenCV's own GTK/X11 highgui backend already pulls
libX11 into the process.

## skywatch — hierarchy contract fix + ML contrail detector (2026-09-07)

Picked back up from `js/skywatch/skywatch.md` (contrail/aircraft
correlation project). Ran out of session budget partway through; two
pieces of follow-up work for next time.

### 1. findContours hierarchy-argument opencv.js contract fix

See BUGS: `findcontours-hierarchy-arg-silently-empty-on-plain-array`.
Mirror the fix already done for the *contours* argument
(`drawcontours-silently-noops-on-plain-array`, fixed this session):

- In `js_imgproc.cpp`'s `js_cv_find_contours`: require `argv[2]`
  (hierarchy) to resolve to a real Mat/UMat when passed, throw a
  `TypeError` otherwise (matching opencv.js, which also requires a Mat
  there - no plain-array convenience). Leave it optional (omitted →
  `cv::noArray()`) same as today.
- Fix `js/cvVectorization.js`'s two call sites that currently pass
  `hierarchy = []` and then read `hierarchy[i][3]`/`hierarchy[i][2]`
  afterward - this never worked (hierarchy was always empty), so this is
  an actual runtime bug fix, not just a contract migration. Reuse
  `js/vectorizer/cv/convert.js`'s existing `hierarchyRows()` helper
  (already handles the Mat 1×N×4 CV_32S parsing correctly) instead of
  hand-rolling the parse again.
- Switch the four vectorizer methods that pass `hierarchy = []` but never
  read it (`skeleton.js`, `canny-contours.js`, `shape-fit.js`,
  `palette-regions.js`) to `new Mat()` for contract cleanliness, even
  though it's a no-op behaviorally for them.
- Rebuild, rerun `tests/unittests/test_imgproc.js` + the vectorizer
  smoke-tests from this session (see conversation for the harness script
  shape - `apply()` each vectorizer method against a synthetic image with
  a rectangle + circle) to confirm nothing regressed.

### 2. ML-based contrail detector, trained via ADS-B weak supervision

The pure-vision detector (`examples/skywatch_contrail_detect.js`) works
on clear-sky frames but can't reliably separate a contrail from linear
natural cirrus on its own (see `js/skywatch.md` phase 3a - LSD and Hough
were both tried and both failed for principled reasons, not tuning gaps).
The user wants to add ML, specifically asked whether an RNN is the right
call, and wants training feedback sourced from real ADS-B position fixes.
Notes for that conversation, not yet designed in detail:

- **Not an RNN for the core detection.** An RNN is for sequential data
  (time series, text); the per-frame contrail-vs-sky decision is a
  spatial/pixel problem, which is what CNN-based semantic segmentation
  architectures (U-Net and descendants) are for. A temporal model (small
  RNN, or more likely just a Kalman filter given how little data this
  project will realistically have) could sit on *top* of per-frame
  detections to track a trail's width growth smoothly across frames, but
  that's a secondary refinement over the primary detector, not the
  detector itself - don't let "we want temporal reasoning" become "so
  the core model must be an RNN."
- **The genuinely good idea in the user's ask: ADS-B as weak/distant
  supervision.** Once phase 4 (ADS-B pixel-matching, see skywatch.md) is
  running, every *matched* trail is a high-confidence, auto-generated
  training label - no manual annotation needed. This is the actual
  labeled-data source: accumulate (frame crop, ADS-B-confirmed trail
  mask) pairs over weeks/months of real operation before training
  anything.
- **Realistic data-volume check needed before committing to a from-
  scratch deep model.** A home ADS-B+camera setup will generate at most
  low hundreds of confirmed-trail frames in the first months. That's not
  enough to train a segmentation CNN from scratch. Options to weigh next
  session: (a) fine-tune a small pretrained segmentation backbone
  (MobileNet-UNet or similar) once enough weak labels exist, deployed via
  ONNX + `cv.dnn.readNetFromONNX` (already used elsewhere in this repo -
  `examples/edge_detection_dexined.js`, `examples/neural-font-sr-
  ESPSCx2.js` - same pattern would apply here: train off-Pi on a machine
  with a GPU, export ONNX, run inference on the Pi); (b) skip deep
  learning for now and instead try a **Frangi/vesselness-style filter**
  (multi-scale Hessian-eigenvalue ridge detector, originally for medical
  vessel segmentation) - not currently bound in this project, would need
  implementing via existing Sobel/second-derivative bindings - which is
  purpose-built for exactly this shape of problem (thin curvilinear
  bright structure against a textured background) and needs zero training
  data; likely the better near-term ROI than jumping straight to ML given
  the data-volume reality. Worth prototyping (b) before committing to (a).
- Whichever direction: keep the ADS-B match as the *ground truth signal*
  a candidate is checked against, not something the vision model has to
  independently reconstruct - the whole reason this project anchors to
  ADS-B in the first place is that vision alone is provably ambiguous on
  a cirrus-heavy sky (see skywatch.md phase 3a's LSD/Hough findings).

## Up next — vectorization pipeline leverage (2026-09-05)

Surfaced while researching a conditioning/vectorization/post-processing
pipeline for `js/vectorizer/` (schematic/line-drawing tracing goal). Not
started — noted for later.

- [ ] **`dnn_superres::DnnSuperResImpl` convenience API** → extend `js_dnn.cpp`.
  ESPCN/FSRCNN currently load through the generic `readNetFromONNX` +
  `Net.forward()` path (works today, see the recent ESPCN fix), but
  opencv.js/native OpenCV also expose a dedicated `setModel(name, scale)` /
  `upsample(src, dst)` API on `DnnSuperResImpl` that hides the
  blob-shape/scale bookkeeping. Bind `dnn_superres::DnnSuperResImpl::create`,
  `.readModel`, `.setModel`, `.upsample`, `.getScale`, `.getAlgorithm` —
  small, self-contained, matches the existing `js_dnn.cpp` factory pattern.
- [ ] **Page dewarping (DocTr/DocUNet-style) for photographed book pages.**
  No OpenCV C++ module does this (checked: no `cv::dewarp`/equivalent
  anywhere in `cv::`) — it has to come from a DNN. Two separate pieces of
  work, in order:
  1. **Export script, `scripts/export_docunet_onnx.py`** (new file, Python
     + PyTorch, not yet started) — fetch a pretrained DocTr or DocUNet
     checkpoint (DocTr: https://github.com/fh2019ustc/DocTr; DocUNet:
     https://github.com/cvlab-stonybrook/DocUNet — confirm which has usable
     pretrained weights and a permissive-enough license before picking one),
     load it in PyTorch, and `torch.onnx.export()` it with a fixed square
     input size (mirror the fixed-`inferSize` pattern `js/cvVectorization.js`
     already uses for its optional segmenter) since this project's `Net`
     binding doesn't exercise dynamic-axes ONNX. Verify the exported graph's
     op set is covered by OpenCV's ONNX importer (or by `ENGINE_ORT`, now
     confirmed available in this build — see BUGS/session notes on
     `cv.dnn.ENGINE_ORT`) before considering this step done.
  2. **Inference + apply, JS-only, no new C++ expected**: once the ONNX file
     exists, loading and running it needs nothing beyond what already
     works — `readNetFromONNX` + `blobFromImage` + `Net.forward()` (proven
     end-to-end against a real transformer-ish segmentation model this
     session). The model's output is a dense per-pixel displacement/flow
     field; applying it is exactly `cv.remap()` (already bound,
     `js_imgproc.cpp`) with the field converted to the `CV_32FC1` x/y maps
     `remap` expects. Only fall back to new C++ if profiling shows the
     field-to-maps conversion needs to leave JS.
- [ ] **Trainable pipeline-permutation evaluator.** Given the ~500 "sensible"
  conditioning/vectorization/post-processing permutations already possible
  in `js/cvVectorization.js` (see session notes 2026-09-05), build a scorer
  that ranks a permutation's polyline output on several qualitative axes
  without needing a ground-truth vector drawing to compare against:
  1. **Reference-free metrics** computed from `(sourceImage, VectorData)`
     pairs - edge fidelity (rasterize the output polylines and compare
     against the source's own Canny/EdgeDrawing edge map via IoU or Chamfer
     distance), compactness (point/shape count relative to image area),
     geometric regularity (angle-histogram peakiness at 0°/45°/90°, a real
     signal for architectural/schematic line art specifically), and noise
     (count of tiny disconnected fragments below some length threshold).
  2. **Trainable layer on top**: a plain linear/logistic regression over
     that metric vector (not deep learning - the label budget will always
     be tiny), fit from a small set of the user's own thumbs-up/down ratings
     on actual pipeline runs (expect ~20-30 rated examples before it tracks
     taste meaningfully). Worth building step 1 alone first and checking
     whether the unweighted/heuristic ranking already correlates with
     judgment before investing in the regression step.
  Not started - scoped in conversation, no code written yet.
- [ ] **os.Worker termination (experimental, C-level, risky).** Checked
  `quickjs-libc.c` (`js_worker_proto_funcs`, ~line 3538): `os.Worker`
  exposes only `postMessage`/`onmessage` - there is no `terminate()` at
  all, and `js_worker_finalizer` (~line 3245) only frees the JS-side
  pipe wrappers on GC, never touches the underlying OS thread. A worker
  thread runs `js_std_loop()` (poll loop) forever until the whole
  process exits - there is currently no way, from JS or from another
  thread, to stop one early, and no cooperative cancellation point
  inside a blocking native call like `cv.Canny()` either.
  This matters for `js/cvThreadPool.js`: when the vectorizer GUI
  supersedes an in-flight pool job (e.g. the user drags a trackbar
  again before the previous run finishes), the worker computing the
  stale result can't be stopped - it runs to completion regardless.
  `ThreadPool.runLatest()` (js/cvThreadPool.js) already discards the
  stale *result* for free, so correctness isn't at stake, only wasted
  CPU/latency on that one worker until it's free again.
  A real fix would need new C-level work in
  `../quickjs/quickjs-libc.c` (a separate project from this repo):
  add `Worker.prototype.terminate()`, most plausibly `pthread_cancel()`
  or a self-pipe/eventfd wakeup + a checked flag in `js_std_loop`'s
  poll. Both are genuinely risky mid-`cv::Mat`/OpenCV-call: a
  `pthread_cancel()` landing inside a C++ destructor stack, or between
  two calls that assume the first completed, can corrupt state or leak
  native buffers (`cv::Mat` ref-counting, ONNX `Net` internals) rather
  than cleanly unwinding. Before attempting it: (1) confirm OpenCV's
  own build enables cancellation points safely (or forces
  `PTHREAD_CANCEL_DISABLE` around risky sections), (2) prototype
  against a throwaway worker doing a slow `cv.dnn.Net.forward()` call
  and verify no corruption across repeated cancel/respawn cycles before
  trusting it in the real pool. Deliberately not attempted in this pass
  - `runLatest()`'s discard-stale-result approach is the shipped fix.

## opencv.js Example Compatibility Gaps (2026-08-17)

Cross-checked every `cv.*` binding referenced by the 80 opencv.js example
pages cataloged in `doc/opencv-js-examples.md` (source: local checkout
`/mnt/data/Projects/opencv/doc/js_tutorials/js_assets/*.html`) against this
project's actual live exports (`Object.keys(cv)` from a built
`build/x86_64-linux-gnu/opencv.so`, OpenCV 5.0.0). 158 unique `cv.*`
symbols are referenced across all 80 pages; 144 already resolve as of
2026-08-24 (`matFromArray` and `Mat.delete()` both shipped). 14 gaps
below still block one or more example pages from running unmodified;
full detail (repro, exact source locations) for each is filed as its own
entry in `BUGS` under the `opencvjs-*` canonical-name prefix.

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

## Already solved, don't rebuild

- **Contour → SVG bezier splines.** No OpenCV algorithm does this (checked the full coverage survey — no such symbol exists, bound or unbound). It doesn't need a library either: `js/cvVectorization.js` (uncommitted) already has a correct Schneider/Graphics-Gems `FitCurves` cubic-bezier fitter (`CurveFitter` — corner detection, chord-length parameterization, Newton-Raphson reparameterization, recursive error-based subdivision) consuming `contour.array` directly, plus `SvgBuilder` for multi-region path output with holes via `fill-rule="evenodd"`. Keep this in JS; it's O(n) per subdivision and QuickJS handles it fine. Only reconsider a C++ port if profiling on a real workload shows it's the bottleneck.
- **Polyline simplification.** All seven `psimpl` algorithms are available under the `cv.psimpl.*` namespace via `js_psimpl.cpp`. Zero-copy across Mat CV_32SC2, Mat CV_64FC2, `PointVector`, and `Point2fVector`, with a plain-array fallback. `tests/unittests/test_psimpl_functions.js` passes 37/37 (7 algorithms x 5 input types, plus error-path tests).
- **Contour → freestanding-function migration and the generic vector infrastructure.** The custom `Contour` class is gone entirely; all 16 shape-analysis functions (`contourArea`, `boundingRect`, `convexHull`, ...) are freestanding, and all 16 opencv.js vector-container types (`MatVector`, `PointVector`, `Point2fVector`, ..., `CharVectorVector`) are implemented via the generic `JSVector<T>` template (`include/js_vector.hpp`) with `push_back`/`get`/`set`/`size`/`Symbol.iterator`/`delete()`. `findContours` accepts `MatVector` or `PointVectorVector` as zero-copy output, alongside a plain-`Array` fallback. See `js_vector.hpp`, `js_vector.cpp`, and `tests/unittests/test_contour_functions.js`/`test_psimpl_functions.js`.
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

- [x] **ximgproc — edge-aware smoothing filters** → `js_ximgproc.cpp` (`cv.ximgproc.*` namespace)
  - `guidedFilter`, `dtFilter`, `l0Smooth`, `jointBilateralFilter`, `bilateralTextureFilter` — smooth flat regions while keeping strong edges crisp; run before Canny/findContours to cut noise contours in gradients/textures (skin, wood grain, fabric).
  - `fastBilateralSolverFilter` — upsamples a coarse/noisy edge or mask to full resolution snapped to real edges; useful now that DNN edge detection (below) can supply that coarse/noisy input.

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

- [x] **dnn — learned edge detection (DexiNed)** → `examples/edge_detection_dexined.js`
  - `DetectionModel`/`SegmentationModel`/`ClassificationModel` were already bound in `js_dnn.cpp` (this TODO item was stale on that point) — the actual blocker was a model file. Fetched `examples/models/edge_detection_dexined/edge_detection_dexined_2024sep.onnx` (OpenCV's own `samples/dnn/models.yml` "dexined" entry, sha1 `f86f2d32c3cf892771f76b5e6b629b16a66510e9`, verified on download).
  - `examples/edge_detection_dexined.js`: `readNetFromONNX` + `blobFromImage` + `Net.forward()` (a 4D 1x1x512x512 blob, so post-processed via a flat sigmoid + min-max stretch over `.data32F` rather than `cv.normalize`/`cv.divide` — those Mat-arithmetic ops don't handle N-D blobs) → `matFromArray` back to a proper 2D Mat → `resize` to the source image size. Verified visually against `tests/smarties.png` — clean, semantically-aware ring outlines, no Canny-style texture noise.

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

## Font tool — `render_font.js` rebuild (2026-08-28)

Goal: pick the tiniest-still-readable pixel fonts for an 84x48 or 128x128
LCD out of `~/Downloads/fonts/` (56 files/dirs currently) plus system fonts
(`fc-list`), and separately pick a few "design opportunity" fonts (artistic
display faces, icon/UI-element fonts) — then convert selected fonts to the
existing PNG+JSON glyph-sheet format, and from there to a bit-packed C
array ready to blit to the display's native byte order.

Current `render_font.js` (single-font PNG+JSON exporter, with an fc-query
metadata pass and a bitmap-vs-vector size-sanity heuristic) is the base to
extend, not throw away — its `queryFontMetadata`/`isCovered`/grid-layout/
size-sanity code is reused as-is by later stages.

Staged so each stage is reviewable and demoable on its own; **do not start
a stage before the previous one is agreed** (this file records the plan,
it doesn't authorize skipping ahead). Sub-stages within an info-gathering
stage can land independently/out of order — call out below where that's
true.

### Stage 0 — directory walk, no analysis yet

- [ ] Point `render_font.js` (or a new `examples/font_scan.js`) at a
  directory (default `~/Downloads/fonts/`) or `fc-list` output, recursing
  into subdirectories, and just list every `.ttf`/`.otf`/`.bdf`/`.pcf`
  file found with its path and file size. No fc-query/FreeType calls yet.
  Proves the traversal + font-format filtering before anything else is
  layered on.

### Stage 1 — terminal truecolor renderer (proof of concept)

**User wants to see this before any deeper info-gathering work.**

- [ ] Render one font's glyph sample to the terminal using 24-bit truecolor
  ANSI escapes (`\x1b[38;2;R;G;Bm`) and the half-block trick (`▀`/`▄` with
  independent fg/bg color per half-cell) to get ~2x vertical resolution
  out of normal terminal cells for an 8-bit-grayscale glyph bitmap.
  Input: a font file + size + a string (default something like
  `ABCabc123!@#`). Output: the rendered sample printed directly to stdout.
  No file I/O, no UI — just `qjsm examples/render_font.js --preview
  <font> --size 16 --text "..."` printing to the terminal.
- [ ] Extend to list samples for *multiple* fonts in one run (e.g. every
  font in a directory), one sample block per font with the filename as a
  header, printed top-to-bottom — this is the "let me list sample
  renderings" step, still no interactive browsing.

### Stage 2 — info-gathering, sub-stage A (cheap, `fc-query` only)

Extends `queryFontMetadata()`. All fields come from a single `fc-query`
call already being made — no new subprocess, no FreeType calls.

- [ ] `family`, `style`, `weight`, `width`, `slant` — for grouping/sorting
  in the eventual browser.
- [ ] `spacing` (`mono`/`dual`/`charcell`/`proportional`) — **highest
  priority signal**: charcell fonts guarantee a fixed advance, which a
  byte-packed glyph table needs. Verify against Stage 2C's empirical
  advance-width check (fontconfig's tag is sometimes wrong).
- [ ] `outline`/`scalable` (already partially used), `fontformat`
  (TrueType/CFF/Type1/BDF/PCF) — BDF/PCF are literal pre-rasterized bitmap
  fonts, a different (simpler) code path than rasterizing an outline.
- [ ] `pixelsize` as a **list**, not one value — multi-strike bitmap fonts
  report several native sizes; report all of them, not just the first.
- [ ] `fontversion`, `foundry`, `decorative` (fontconfig's own
  "display/art font, not body text" flag — a cheap first pass at the
  "artistic" bucket), `lang`/`capability` (rough language coverage).

### Stage 2 — info-gathering, sub-stage B (font-program-level metrics)

**Plan changed (2026-08-28): no native FreeType/HarfBuzz C++ binding.**
The original idea here was a direct FreeType wrapper in `qjs-opencv`
itself. Decided against it — checked whether that loses anything, and it
doesn't: every fact on the list below (and the two extra ones the
original note flagged as FreeType-only, hinting-program presence and
`fvar` axes) is available from `fontTools` (already installed here,
`pip show fontTools` → 4.61.1), shelled out to from `render_font.js`
exactly the way `fc-query` already is — see `queryFontProgramInfo()`.
A native binding would only add rendering-*path* detail (which rasterizer
code path FreeType picks, exact hint bytecode execution) that nothing
here needs. Implemented:

- [x] Units-per-EM (`head.unitsPerEm`).
- [x] Glyph count (`maxp.numGlyphs`), outline format (`CFF ` vs `glyf`
  table presence).
- [x] Hinting instructions present (`fpgm`/`prep` bytecode non-empty).
- [x] Variable-font axes (`fvar.axes`), if present.
- [x] Color-glyph tables present (`COLR`/`CPAL`/`sbix`/`CBDT`/`CBLC`).
- [x] **New, not on the original list**: a best-effort "pixel grid unit"
  for scalable fonts — GCD of glyph outline point coordinates sampled
  across up to 60 glyphs, converted to a candidate crisp pixel size via
  `unitsPerEm`. Added to answer the actual "why does this font render
  antialiased" question (see Stage 2C below) — ascender/descender/
  line-gap from the original list turned out not to be needed for that
  and were dropped, not implemented.

### Stage 2 — info-gathering, sub-stage C (pixel-level, rendered-glyph analysis)

Extends the existing `blockinessScore`/`distinctMidgrayLevels` heuristics
— same idea (render, then measure the Mat), applied per-glyph instead of
whole-sheet.

- [x] **Crisp-render-size search (`findCrispSize()`), implemented
  2026-08-28.** The concrete problem that prompted this: fonts in
  `~/Downloads/fonts/` that are genuinely pixel-art fonts (blocky glyph
  *design*) are still ordinary scalable TrueType outlines under the
  hood — not embedded bitmap strikes — so they only render bilevel/crisp
  at the specific pixel size(s) that happen to make their outline's
  design grid land on exact integer pixels; any other size antialiases
  like any other vector font, and there's no metadata field that just
  states that size. Bounded search (≤10 renders, run only for the one
  font open in the detail view, never for a whole directory): candidates
  are Stage 2B's glyph-outline grid hint (when it lands in a plausible
  4-64px range) plus a fixed list of common pixel sizes
  (`8,10,12,14,16,20,24,32`); each candidate is rendered once and scored
  with the exporter's existing `distinctMidgrayLevels()` heuristic;
  candidates below `BITMAP_LEVEL_THRESHOLD` (40) count as crisp. An
  embedded-bitmap-strike font (fc-query `pixelsize`) skips the search
  entirely — already known-crisp at its one native size. Verified against
  real fonts: `monogram.ttf`/`LowRes 3x4.ttf`/`3x4dot.ttf` all found
  crisp sizes (multiples of 8px in `LowRes 3x4.ttf`'s case, matching its
  name); `DejaVuSansMono.ttf` (an ordinary vector font, correctly) found
  none in range.
- [ ] Tight ink-bbox vs advance-box padding per glyph — how many empty
  rows/columns surround the glyph inside its cell; high padding wastes
  pixels on a tiny LCD.
- [x] Bilevel check — implemented as part of `findCrispSize()` above
  (per-candidate-size, not per-glyph yet - a per-glyph breakdown at one
  fixed size is still open if it turns out to matter).
- [ ] Stroke weight / ink density — average ink-pixel % inside the tight
  bbox; flags weights too light/heavy to survive a 1-bit threshold.
- [ ] Counter (enclosed-hole) size in glyphs like `e a o g` — flood-fill
  background, find holes fully enclosed by ink, measure smallest area.
  Likely the best automatable proxy for "readable vs mush" at tiny sizes.
- [ ] Confusable-pair check — pixel-diff `1`/`l`/`I`, `0`/`O`, `5`/`S` at
  the target size; flag fonts where these render identically.
- [ ] Baseline-consistency check across a full glyph run (catches broken
  or hand-hacked bitmap fonts).
- [ ] Empirical min/max/average advance width, to cross-check fontconfig's
  `spacing` claim from Stage 2A.

### Stage 2 — info-gathering, sub-stage D (special-range coverage)

- [x] **Coverage summary against a curated named-block list
  (`summarizeCoverage()`), implemented 2026-08-28** as part of the
  detail-view work below — Basic Latin, Latin-1, Latin Extended-A/B,
  Greek, Cyrillic, General Punctuation, Box Drawing, Block Elements,
  Braille Patterns, Private Use Area, Powerline/Nerd symbols, Emoji, each
  as covered/total/% against fc-query's charset ranges (already parsed by
  `queryFontMetadata`). Not a full Unicode block database — deliberately
  scoped to blocks relevant to this tool's actual use (LCD text/UI), same
  list this stage originally proposed.
- [ ] Box-drawing (`U+2500-257F`) / block-element (`U+2580-259F`)
  presence, and whether *these specific* glyphs render crisp/bilevel even
  when the rest of the font antialiases — needed for any UI-chrome use.
- [ ] Braille patterns (`U+2800-28FF`) presence — enables a dense
  sub-pixel-style rendering trick on 1-bit displays.
- [ ] Emoji-range presence and whether it resolves color (via Stage 2B's
  color-table check) or monochrome fallback.
- [ ] Powerline (`U+E0A0-E0D4`) and Nerd Font PUA icon sub-block presence
  — matches the "UI build elements" font category from the original ask.
- [ ] General Private-Use-Area density vs Latin/ASCII density — a font
  that's PUA-heavy but Latin-sparse is very likely an icon font, not a
  text font; surface this as an automatic classification signal.
- [ ] Roll sub-stages A-D up into one auto-tag per font: `pixel-mono` /
  `pixel-proportional` / `icon-set` / `display-art` / `unknown`, so the
  eventual browser can filter/sort by it without anyone eyeballing first.

### Stage 3 — ncurses-style TUI browser

No ncurses binding exists in this codebase; default to hand-rolled raw
ANSI (alt-screen, cursor positioning, raw stdin key reads) rather than
adding a new native dependency, unless a later stage shows that's not
enough.

- [x] Up/down/pageup/pagedown scrollable list of fonts gathered from the
  three Stage 0 sources, each showing a truecolor half-block sample.
  Space marks (not yet wired to anything — Stage 4). Not yet annotated
  with a Stage 2D auto-tag (that roll-up bullet is still open).
- [x] **List preview now uses each font's own detected crisp size and a
  per-font sample text, implemented 2026-08-28.** `#preview()` calls
  `#ensureDetail()` (runs `gatherDetailInfo()`/`findCrispSize()`) the
  first time a font's row is actually drawn - i.e. detection happens as
  part of reading/scrolling the list, not gated behind Enter - and uses
  `crisp.best.size` instead of the fixed `--size` default once known.
  `pickSampleText()` substitutes a sample built from the font's own
  covered codepoints when the requested sample text has a character the
  font doesn't map (avoids showing FreeType's `.notdef` "tofu" box in a
  list of *font* previews, which reads as "this font is broken" rather
  than "this sample string doesn't fit this font"). Deliberately still
  lazy/per-row, never the whole directory up front - only rows that
  actually get drawn incur the up-to-~10-renders cost, so opening a
  large `--fclist` match set doesn't stall on fonts you never scroll to.
  `main()` prints a one-line heads-up before entering the list, since the
  very first screenful's rows haven't been drawn/cached yet and pay that
  cost synchronously.
- [x] **Enter → combined glyph-map + info-panel view, implemented
  2026-08-28 (redesigned from an earlier two-separate-screens version
  the same day) — per direct instruction, "font-info and glyph viewer
  should really share the screen, with font-info being a panel overlay
  and the glyph viewer in the back".** The 2D-pannable glyph map
  (`#drawGlyphMapFrame()`) is always the full-screen background and stays
  interactive; the font-info panel (`#drawInfoPanel()` - overview/
  render-size/coverage, deliberately scoped down from "full metadata
  dump" the same way it always was) draws as a bordered, centered overlay
  box on top of it via terminal.js's new `Screen.box()`, toggled with `i`.
  Map: 16 codepoints/row (`GLYPHMAP_COLS`) over the font's actual covered
  codepoints (index-packed via `flattenCoveredCodepoints()`, not raw
  codepoint value - most fonts here have sparse coverage with big gaps),
  rendered at `findCrispSize()`'s best-guess size - confirmed on real
  fonts from `~/Downloads/fonts/` to look correct at that size. Up/down/
  pageup/pagedown scroll rows; left/right shift which of the 16 columns
  is leftmost (16 cells at any real cell width is normally wider than one
  terminal). Esc/Enter/q returns to the font list.
  **Still open, deliberately deferred as out of scope for this pass:**
  truecolor rendering of the special ranges themselves (box-drawing/
  block/braille/emoji samples) as a distinct feature from the general
  glyph map, the auto-tag roll-up, and the deeper per-glyph Stage 2C
  items (padding, stroke weight, counter size, confusable pairs, baseline
  consistency).

**New reusable terminal.js primitives added for this** (`quickjs/
qjs-modules/lib/terminal.js`, not `qjs-opencv`-local — flagged for the
qjs-modules session too): `Screen.fillRect()`/`Screen.box()` (an
ncurses/dialog-style bordered "window", filling its interior first so it
occludes whatever else was drawn earlier in the same frame - draw order
is z-order for a plain terminal, there's no real compositor to manage)
and a free `centeredRect(termCols, termRows, width, height)` matching the
curses `newwin()`-centered-on-screen idiom. `BOX_SINGLE`/`BOX_DOUBLE`
export the border character sets. Kept general-purpose, not
render_font.js-specific, per the earlier "keep terminal primitives at one
point in terminal.js" instruction. Also added `hslToRgb(h, s, l)` (plain
color-wheel math - h is the wheel angle) for the same reason: the info
panel's pastel scheme and the glyph map's color-mode toggle (below) both
need it, and it's no more app-specific than the RGB truecolor helpers
already there.

- [x] **Info panel color scheme + Unicode line-art/pictograms, implemented
  2026-08-28.** Two pastel hues, each a light and dark shade: `PANEL_HUE`
  (cool teal-blue) is the panel's structural color - light for the
  border/title, dark for the background fill that occludes the glyph map
  behind it; `ACCENT_HUE` (warm coral) highlights specific values inline
  within the body text (`panelHighlight()` - a recommended size, a
  coverage percentage, a hinting yes/no) via `Screen.fg()`/`bg()` set
  before `Screen.box()`/each line write, always returning to the panel's
  own base colors afterward rather than a blanket reset (which would also
  wipe the background tint). Content reformatted to use `┄`/`▸`/`✓`/`✗`/
  `⚠`/`▦`/`◆`/`·` instead of `--` headers, `->` arrows, and parenthetical
  asides, per direct instruction - picked from Box Drawing/Geometric
  Shapes/general punctuation (near-universal monospace font coverage)
  rather than emoji, given the earlier `screen`/`TERM=screen-256color`
  terminal-compatibility issue found in this same tool.
- [x] **Toggleable glyph-map color mode, implemented 2026-08-28.** `c` in
  the combined detail view switches the glyph map from plain grayscale to
  `colorWheelPalette(hue)` - maps each pixel's intensity to a point on one
  hue's arc of the color wheel (dark/desaturated at background level,
  vivid/bright at ink level) instead of `[v,v,v]` gray, so the existing
  antialiasing-level gradient becomes a real color gradient without losing
  the light/dark contrast the glyph shape depends on. Each time the mode
  is turned back on it advances the hue by the golden angle
  (~137.508deg, `FontListBrowser#nextHue`) - a well-known technique for a
  maximally-spread, non-clustering, non-repeating sequence of hues - so
  repeated toggling (even across different fonts in one session) keeps
  showing a genuinely different palette, per "a different (and calculated
  to color wheels) color palette every time".
- [x] **Glyph-map size cycling on `-`/`=`/`+`/`_`, implemented 2026-08-28.**
  `-`/`=` step through `goodSizesFor(crisp, fallbackSize)` — the sizes
  `findCrispSize()` actually tried, plus the base/`x2`/`x4` "obviously
  good" multiples, deduped/sorted/floored at 2px, cached once per font
  alongside the rest of `#prepareGlyphMap()`'s state. `+`/`_` (the same
  physical keys with shift held — `readKey()` can only tell them apart by
  the different byte a real keyboard sends, there's no separate modifier
  flag) instead step `gm.size` by ±0.5px directly, not through the
  candidate list. Both paths go through `#tryChangeGlyphMapSize()`, which
  clamps to a 2px floor and rejects (no-ops, leaving the current size
  alone) any target where `cellWidth * 2 > maxCols` — i.e. fewer than 2
  glyph cells would remain visible — per direct instruction, "never scale
  it so big that not even 2 glyphs are visible anymore." Verified with a
  standalone logic test mirroring `goodSizesFor()`/`#tryChangeGlyphMapSize
  ()` against a synthetic `findCrispSize()` result: coarse cycling stops
  cleanly at the candidate list's ends (`undefined`, not a wrap or crash),
  fine-stepping down floors at 2px and then correctly no-ops rather than
  going negative, and an oversized coarse target is rejected instead of
  applied. **Found along the way, not fixed (logged in `BUGS` as
  `opencv-freetype-fontscale-truncated-to-int`):** fractional pixel sizes
  are silently truncated to the integer size somewhere inside upstream
  opencv_contrib's FreeType2 module (`TextStyle(f, 8.5).size('A')`
  measures identical to `size 8`), not in this project's own binding code
  (`js_draw.cpp` passes the `double` through untouched) — the 0.5px fine
  step is implemented and tracked/displayed as requested regardless, since
  declining silently over an upstream limitation the user didn't ask about
  would be worse than shipping a control whose finest steps may not always
  visibly change the render.

### Stage 4 — PNG+JSON export of selected fonts

- [ ] For every font selected in Stage 3, run (in effect) today's existing
  `render_font.js` export path to produce the `.png` + `.json` sidecar
  pair, unchanged in format from what's already produced.

### Stage 5 — second browser, over generated sidecars

- [ ] Separate top-down scrollable menu listing already-generated
  `<name>@<size>.json`/`.png` sidecar pairs (not raw font files); select
  a subset to carry into Stage 6.

### Stage 6 — bit-packed C export

- [ ] From the Stage 5 selection, emit a bit-in-bytes-encoded font-data
  block in embedded C (`static const uint8_t font_x[] = {...}`), already
  in the byte/bit order the target display expects (row-major vs
  column-major, MSB/LSB-first — needs the specific display's native
  format confirmed before writing this, not assumed).

## API discrepancies vs opencv.js (shared surface only)

**IMPORTANT: When fixing opencv.js compatibility issues, always update all client scripts.** After changing a C++ binding to match opencv.js behavior, you MUST update every JavaScript file that uses the affected function:
```bash
grep -r -l "^import.*'opencv" examples/ tests/ *.js ../*.js
```
Update the call sites to use the opencv.js-compatible form, even if qjs-opencv retains backward compatibility (e.g., supporting both 1-arg and 2-arg signatures). The goal is to make client scripts as opencv.js-portable as possible.

Investigated 2026-08-17 (previous pass: 2026-08-12) by comparing `doc/opencv-js-api.md` (the official opencv.js binding surface, compiled from OpenCV 5.0.0's `platforms/js/opencv_js.config.py` + `modules/js/src/core_bindings.cpp` + `helpers.js`) against this project's own bindings, function-by-function, for names that exist **on both sides**. This is not a coverage comparison — qjs-opencv deliberately binds far more of `cv::` than opencv.js's hand-curated browser whitelist (see Tiers above). It's a divergence audit: where the same-named call exists in both, does it behave the same way? Anyone porting opencv.js snippets/tutorials into this project's `qjs` REPL will hit these.

The codebase moved substantially since the 2026-08-12 pass: `js_contour.[ch]pp` (the custom `Contour` class), `js_point_iterator`, `js_line_iterator`, and `js_slice_iterator` were removed entirely; `findContours` now natively supports `MatVector`/`PointVectorVector` output; `drawContours` (plural) now threads a real `index`/`hierarchy` through instead of hardcoding `index=0`; `cv.Range` is now a real constructible class; `Mat.diag()`, the seven `.dataXX` typed views, the seven `<type>At()` accessors, and the seven `<type>Ptr()` accessors all shipped; `Scalar.all()` and `barcode_BarcodeDetector` both shipped. Several other entries from the old pass are unchanged — re-verified below, not just carried forward blindly. Entries marked "carried forward, not re-verified" cover files with no commits since 2026-08-12 — low risk of drift, flagged transparently rather than silently repeated as fresh findings.

Grouped by how badly a naive port breaks.

**Not a divergence, logged because it caused real confusion (2026-08-28):**
user reported treating `mat.dims` as a per-dimension shape array (`dims[1]`,
`dims[2]`, `dims[3]`) in Gemini-authored ESPCN super-resolution post-
processing code, and had to switch to `.size()` to get it working. Checked
both sides: this project's `.dims` (`js_mat.cpp:2070`, `PROP_DIMS`) is
correctly a scalar — `m->dims`, matching native C++ `cv::Mat::dims` (the
rank) — it was never an array here. opencv.js has **no `.dims` property at
all** (`doc/opencv-js-api.md:100-112`); its N-D shape accessor is
`.matSize` (`int[]`) or `.size()`. This project's `.size()`
(`js_mat_size_value()`, `js_mat.cpp:1408-1428`) already matches opencv.js:
for a normal 2D mat it's a `Size`-like object also indexable by dimension
(`size[0]`, `size[1]`, ...), and for a true N-D mat (`rows`/`cols == -1`,
e.g. a 4-D DNN output tensor) it's a plain `[dim0, dim1, ...]` array — the
`[1, 3, H, W]` shape the ESPCN code needed. So nothing to fix on either
side; the original code's bug was reaching for `.dims` (rank, absent from
opencv.js) instead of `.size()`/`.matSize` (shape, present on both). The
user's own fix was already the opencv.js-correct one.

### Silently wrong results (no exception, no obviously-missing output — the dangerous category)

No systematic new pass has been done recently to look for further
instances of this category - worth another audit pass rather than
assuming it's now exhaustively clean.

*(`Mat.diag()`, `Mat.data`/`.data8S`/`.data16U`/`.data16S`/`.data32S`/`.data32F`/`.data64F`, all seven `<type>At()` accessors, all seven `<type>Ptr()` accessors, and `StringVector`/`DMatchVectorVector`/`KeyPointVectorVector` from the 2026-08-12 audit's "not bound" list are now **implemented** — `js_mat.cpp:2051-2101`, `js_vector.cpp` — and removed from this list.)*

### Different return shape / calling convention (throws or misbehaves on an opencv.js-style call, but obviously so)

- **`kmeans`'s `criteria` argument shape differs, and no `TermCriteria` class exists anywhere.** *(unchanged — still live)* `js_cv.cpp:1068-1092`: `criteria` is read via `js_array_to(ctx, argv[3], crit)` into a plain `std::vector<double>` (needs `crit.size() >= 3`), not opencv.js's `TermCriteria {type, maxCount, epsilon}` object. Grepped the whole tree for a `TermCriteria` JS class — none exists; every internal use (`js_fisheye.cpp`, `js_calib3d.cpp`, `js_cv.cpp`, and now `CamShift`/`meanShift`'s `js_imgproc_track`) hardcodes its own C++-side `cv::TermCriteria`, none of it constructible/settable from JS.
- **`Mat.empty`/`.continuous` are JS accessor properties, not callable methods.** *(unchanged — still live)* `js_mat.cpp:2044,2046` (`JS_CGETSET_MAGIC_DEF`) — `mat.empty()`/`mat.continuous()` both throw (`TypeError: not a function`); must read `mat.empty`/`mat.continuous` with no parens. `.isContinuous()` (the opencv.js name) doesn't exist as a property or method name at all — only the renamed `.continuous` getter does.
- **No `cv.Scalar` constructible type, and it doesn't subclass `Array`.** *(partially fixed since 2026-08-12)* `Scalar.all(v)` now exists (`js_cv.cpp:1440`, `:2605-2606`) and returns a uniform fill, matching opencv.js. But there's still no `new cv.Scalar(...)` constructor — only the `js_scalar_read`/`js_scalar_new` helpers operating on plain arrays/`Float64Array` — and the return type is a `Float64Array`, not an `Array` subclass (`scalar instanceof Array === false` here, `true` in opencv.js).
- **`SimpleBlobDetector`'s constructor still ignores/has no way to take a `Params` argument.** *(carried forward, not re-verified)*
- **`calibrateCameraExtended` doesn't exist as a separate name — folded into `calibrateCamera`.** *(carried forward, not re-verified)*
- **`findHomography`'s point inputs go through a custom point-array reader, not a generic `InputArray`.** *(carried forward, not re-verified)*
- **`DescriptorMatcher.match()` dispatches on `argc` rather than argument type** — the `mask` argument (`js_feature2d.cpp`'s `DESCRIPTOR_MATCHER_MATCH`) is only read, and matching only actually runs, when `argc > 3`; calling `matcher.match(query, train, matches)` (3 args, `mask` omitted — a perfectly valid opencv.js call) silently does nothing and leaves `matches` empty, no exception. *(newly split out as its own entry — previously bundled into the now-fixed "static function table is empty" entry above; found while verifying the `DescriptorMatcher.create()` fix below.)*

`BFMatcher.create(normType, crossCheck)` specifically is still not
separately bound — `new cv.BFMatcher(...)` and
`DescriptorMatcher.create('BruteForce'-family-string)` both work as the
two ways to construct one, but the literal opencv.js
`BFMatcher.create(...)` spelling still throws; low priority given the
two working alternatives.

### Vector container types — remaining gaps

All 16 opencv.js vector-container types exist (see "Already solved, don't rebuild" above); `.delete()` is a no-op on all of them.

- **Still not migrated to `MatVector`/vector-of-vectors conventions:** `split`/`merge` (`js_cv.cpp:194,333`, re-verified 2026-08-17 — still plain JS `Array` of `Mat`s). `HoughLines` (`js_imgproc.cpp:464`, re-verified 2026-08-17 — still requires a plain JS `Array` for `lines`, inconsistently with `HoughLinesP`/`HoughCircles`).

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

## jsbindings.hpp Dead Code Audit (2026-08-27)

`include/jsbindings.hpp` is almost entirely `static inline` free functions and
function templates, which makes standard dead-code detection blind: neither
`nm`/`objdump` linker-symbol analysis nor `-Wunused-function` (GCC *or*
Clang) can see any of it - see `BUGS`: `std-file-methods-broken-after-opencv-import`
is unrelated, but `wunused-function-cant-see-header-only-static-inline` and
`js_iterable_to-array-overload-uncallable` are directly relevant background.

What worked: a Clang `-ast-dump=json` scan (`-ast-dump-filter=js_`/`dump`,
`-fsyntax-only`) run across all 54 of this project's `.cpp` translation
units, reading each declaration's own `isUsed` flag (and, for templates,
whether any instantiation child ever appears) rather than relying on a
diagnostic. Aggregated by **(file, line)**, not by name - `jsbindings.hpp`
has real overload sets sharing one name at different lines, and a couple of
false leads below only surfaced by checking exact line/signature matches
instead of a plain name grep.

**Confirmed dead (independently verified by full-repo grep - zero callers anywhere):**

- `js_number_read<int64_t>` (line 111, explicit specialization) - every real
  call site passes fields that resolve to `int32_t`/generic-`double` paths;
  nothing passes an actual `int64_t*`.
- `js_object_tostringtag(ctx, obj)` - the 2-argument overload only, at line
  326. The 3-argument overloads (`..., JSValue value` at 334, `..., const
  char* str` at 341) are heavily used (25+ call sites in `js_feature2d.cpp`)
  and must stay.
- `dump(const ArrayBufferProps&)` (line 433) - distinct from the several
  other `dump()` overloads in `util.hpp`/`js_typed_array.hpp` that *are*
  called.
- `js_is_iterator` (line 626) - only its own declaration appears anywhere;
  even the newer `js_iterator`/`js_iterator_range` iterator-adapter code
  never calls it.

**False positive - do NOT remove:**

- `js_iterable<T>::to_array` (line 873) - the scan flagged it dead, but it's
  actually called (via the `js_iterable_to(ctx, arr, std::array<T,N>&)`
  overload, which real code calls from `js_line.hpp:96` and
  `src/jsbindings.cpp:28`). Root cause: it's a *member* function template of
  a class template (`js_iterable<T>`) - its real instantiations live under a
  separate `ClassTemplateSpecializationDecl` subtree that this scan never
  walked (it only followed `FunctionDecl`/`FunctionTemplateDecl` nesting, not
  `CXXMethodDecl`). Corollary: `to_vector`/`to_scalar` (siblings in the same
  class, not flagged dead) were *not* independently verified either - their
  "used" status might be coincidental rather than confirmed. **This scan
  cannot be trusted for class members at all** - only evaluated free
  functions/free function templates here.

**Flagged dead by the scan, not yet independently verified - check each
by exact signature/line (not just name) before touching, given the
`js_object_tostringtag` overload trap above:**

- `js_range_size`, `js_range_empty`, `js_range_valid` (lines 73-75) - only
  prototypes here; bodies are defined out-of-line in `src/jsbindings.cpp`.
  Found the definitions but haven't confirmed there's no caller anywhere.
- `js_object_property<T>` (283)
- `js_arraybuffer_range<T>` (360)
- `js_arraybuffer_slice` (397)
- `js_arraybuffer_from<T,N>(std::array<T,N>&, ...)` (409) - overload set:
  its own body calls a *different* `js_arraybuffer_from` (iterator-pair)
  overload internally, and a plain grep for the name found 5 references
  elsewhere - need to confirm which overload those actually call before
  trusting this one is dead.
- `js_arraybuffer_props` (447)
- `js_is_scalar` (521)
- `js_iterable_to(ctx, arr, std::vector<T>&)` (934)
- `js_iterable_to(ctx, arr, T (&out)[N])` (946) - note: this signature has
  *already changed on disk* (not by this session) from the broken by-value
  form (`T[N] out`) documented in `BUGS`
  (`js_iterable_to-array-overload-uncallable`) to the correct
  reference-to-array form. Re-verify and update/close that `BUGS` entry -
  the exact bug it describes may no longer exist verbatim, though the
  function could still be genuinely unused now that it's at least callable.
- `js_atom_is_index`, `js_atom_is_length`, `js_atom_is_symbol` (971, 1007, 1017)
- `js_scalar_new<T>` (1131)
