# TODO — binding backlog

Prioritized by leverage for plot-cv's actual pipeline (video source → denoise/edge-detect → contour → SVG for laser cutting), not by raw OpenCV API surface. Derived from a `scripts/binding_coverage.js` run against `build/x86_64-linux-debug/opencv.so` (OpenCV 4.13.0), filtered to `cv::`-namespace symbols only.

Regenerate the underlying data with:
```bash
qjsm scripts/binding_coverage.js --module=build/x86_64-linux-debug/opencv.so \
  --lib-dir=/opt/opencv-4.13.0-x86_64/lib --namespace=cv --verbose --out=cov.txt
```

## Tier 1 — bind next

Small, self-contained additions to files that already exist. Each one either completes a pipeline stage that's currently a dead end, or gives Canny→findContours a materially better input image.

- [ ] **photo module** (0/30 bound) → new `js_photo.cpp`
  - `pencilSketch`, `stylization`, `detailEnhance`, `edgePreservingFilter` — non-photorealistic-rendering filters that convert a photo *directly* into line art; worth an A/B test against the current Canny+findContours approximation.
  - `fastNlMeansDenoising`/`fastNlMeansDenoisingColored` — strips webcam sensor noise that currently becomes spurious tiny contours in the SVG output.
  - `inpaint` — removes dust/scratches from scanned source images before vectorizing.

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
