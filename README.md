# qjs-opencv

OpenCV bindings for [QuickJS](https://bellard.org/quickjs/), compiled into a single loadable module `opencv.so` (plus optional per-class `quickjs-*.so` modules). Import it as:

```js
import * as cv from 'opencv';

let viewport = new cv.Size(640, 480);
let mat = new cv.Mat(viewport, cv.CV_8UC3);
let cap = new cv.VideoCapture(0);

cap.read(mat);
cv.imshow('qjs-opencv', mat);
```

This is the C++/JS sister project to [`plot-cv`](https://github.com/rsenn/plot-cv), which converts contours from a video source to SVG for LaserWeb4. Combined with the QuickJS REPL, it's a live-coding environment for OpenCV pipelines: ES2020 syntax, no build step, no copies of image data on the JS/C++ boundary.

## Design

- **No copies, mutable, finalizers do the work.** A `cv.Mat` is backed by a `cv::Mat`; iteration yields `Float64Array(4)` views into the underlying buffer. `cv.Contour` is a `std::vector<cv::Point3d>` exposed directly as an iterable ArrayBuffer.
- **Interchangeable array types.** Most free functions taking `cv::InputArray` / `cv::OutputArray` / `cv::InputOutputArray` accept `cv.Mat`, `cv.Contour`, or a plain TypedArray interchangeably (see `JSInputArgument` / `JSImageArgument` in `include/jsbindings.hpp`).
- **Iterators yield views, not copies.** `MatIterator` yields `Float64Array(4)`, `PointIterator` yields `cv.Point`, `SliceIterator` partitions or slides across an ArrayBuffer yielding offset TypedArrays.
- One `.cpp` file per OpenCV concept — see `CLAUDE.md` for the full architecture breakdown.

## Build

```bash
. ./cfg.sh
prefix=/usr/local TYPE=Release cfg -DOpenCV_DIR=/opt/opencv-4.7.0-x86_64/lib/cmake/opencv4
make -C build/x86_64-linux-gnu -j$(nproc)
```

Builds against OpenCV 4.2.0+ (NONFREE modules needed for `LineSegmentDetector`), against vanilla QuickJS or the [rsenn/quickjs](https://github.com/rsenn/quickjs) fork with C module search paths. Cross-compiles to aarch64, x86_64-w64-mingw32, and wasm32 (emscripten/clang) via `cfg.sh` toolchain functions (`cfg-clang`, `cfg-mingw64`, `cfg-wasm`, `cfg-aarch64`, `cfg-musl*`, `cfg-android*`). See `CLAUDE.md` for the full option/flavor matrix.

## What works

Confirmed by both the C++ source under `js_*.cpp` and by what's actually exercised in `tests/*.js`.

**Core value types** — `Mat`, `UMat`, `Contour`, `Point`, `Rect`, `RotatedRect`, `Size`, `Line`, `KeyPoint`, `Matx`, `Affine3`, plus their iterators (`MatIterator`, `PointIterator`, `LineIterator`, `SliceIterator`).

**imgproc** — the bulk of the classic pipeline is bound and tested: `Canny`, `findContours`/`drawContours`, `HoughLines(P)`, `HoughCircles`, `cvtColor`, `threshold`/`adaptiveThreshold`, `blur`/`GaussianBlur`/`bilateralFilter`/`medianBlur`, `dilate`/`erode`/`morphologyEx`, `warpAffine`/`warpPerspective`/`resize`/`remap`, contour metrics (`contourArea`, `arcLength`, `approxPolyDP`, `convexHull`, `minAreaRect`, `fitEllipse`, `moments`/`HuMoments`), `watershed`, `grabCut`, `distanceTransform`, `floodFill`, `calcHist`, `connectedComponents(WithStats)`.

**draw / highgui** — `Draw` (circle/ellipse/contour/line/polygon/rect/keypoints), text via FreeType (`putText`, `loadFont`, `getTextSize`), `Window`/`imshow`/trackbars/mouse callback, all exercised through the `js/cvHighGUI.js` wrapper.

**calib3d / fisheye** — `calibrateCamera`, `findHomography`, `findChessboardCorners(SB)`, `estimateAffine2D/3D`, the full `fisheye::*` distortion/rectification set.

**video I/O** — `VideoCapture`, `VideoWriter` (FFMPEG), `MOG2`/`KNN` and the `bgsegm` background subtractor family (`CNT`, `GMG`, `GSOC`, `LSBP`, `MOG`).

**dnn** — `Net`, `blobFromImage(s)(WithParams)`, `NMSBoxes`, and `readNet`/`readNetFrom{Caffe,Darknet,ONNX,Tensorflow,TFLite,Torch,ModelOptimizer}` — loading and running pre-trained models works; the training/layer-introspection API does not.

**ximgproc / xphoto** — thinning, structured edge detection, superpixel segmentation (`SLIC`/`SEEDS`/`LSC`), selective search segmentation, `FastLineDetector`, `EdgeDrawing`, `weightedMedianFilter`, white balance (`Grayworld`/`LearningBased`/`SimpleWB`).

**Persistence & misc** — `FileStorage`/`FileNode` (YAML/XML/JSON), `CommandLineParser`, `CLAHE`, `Subdiv2D` (Delaunay/Voronoi), `TickMeter`, OpenGL interop (`ogl::Buffer`/`Texture2D`, `imshow` with `WINDOW_OPENGL`).

**In-tree algorithms not from OpenCV** — skeletonization, pixel-neighborhood tracing, palette generation/reduction, low-bit-depth PNG/GIF encoding (`algorithms/`, `gifenc/`, `giflib-turbo/`).

## What's missing

The binding surface is intentionally driven by what `plot-cv` pipelines need, not full API parity. Whole OpenCV modules are currently **unbound** (0% coverage as of the last `binding_coverage.js` run): `alphamat`, `aruco` (marker *detection* — only the debug-draw helpers are bound), `bioinspired`, `ccalib`, `datasets`, `dnn_objdetect`, `dnn_superres`, `dpm`, `face`, `flann`, `fuzzy`, `gapi`, `hdf`, `hfs`, `img_hash`, `line_descriptor`, `mcc`, `ml`, `optflow`, `phase_unwrapping`, `photo` (inpainting/HDR/seamless cloning), `plot`, `quality`, `rapid`, `reg`, `rgbd`, `saliency`, `sfm`, `shape`, `signal`, `stereo`, `stitching`, `structured_light`, `superres`, `surface_matching`, `text` (OCR), `tracking` (the legacy tracker API — `MOG2`/`KNN` background subtraction is bound via `video`, single-object trackers are not), `videostab`, `wechat_qrcode`, `xfeatures2d` (SURF/SIFT-adjacent extras), `xobjdetect`.

Partially bound modules worth knowing about: `features2d` (only `drawKeypoints` — the detector/descriptor/matcher classes themselves, e.g. `ORB`, `BFMatcher`, are not exposed as `cv.*` bindings, unlike `js_feature2d.cpp`'s name might suggest), `objdetect` (only ArUco draw helpers — no `CascadeClassifier`/`HOGDescriptor`/QR code detection), `video` (background subtractors only — no optical flow, Kalman/particle filters, or object tracking).

## Binding coverage

`scripts/binding_coverage.js` measures this precisely rather than by inspection: it diffs `opencv.so`'s imported (undefined) mangled symbols against each `libopencv_*.so`'s exported symbols, classifying each as an implemented/missing class constructor or free function.

```bash
qjsm scripts/binding_coverage.js --lib-dir=/opt/opencv-4.13.0-x86_64/lib --out=report.txt
```

Last run against `build/x86_64-linux-gnu/opencv.so` (OpenCV 4.13.0):

| | implemented | total | % |
|---|---|---|---|
| classes | 16 | 342 | 4.7% |
| functions | 383 | 2873 | 13.3% |
| **overall** | **399** | **3215** | **12.4%** |

Modules with the highest bound fraction: `imgproc` (107/207, 52%), `bgsegm` (5/10, 50%), `highgui` (25/41, 61%), `freetype` (1/2, 50%), `ximgproc` (34/90, 38%), `xphoto` (4/11, 36%), `core` (155/811, 19%), `dnn` (35/187, 19%), `calib3d` (20/109, 18%).

**Read these numbers with caveats, not as a scorecard:**
- The method only counts symbols that are *exported from a shared object*. OpenCV's small value types (`cv::Point_<T>`, `cv::Rect_<T>`, `cv::Size_<T>`, `cv::Scalar_<T>`, ...) are header-only templates with no exported constructor symbol at all, so `cv.Point`/`cv.Rect`/`cv.Size` — fully implemented here — don't register as "implemented classes" in this report.
- Classes exposed only through a static `::create()` factory (common OpenCV pattern for `Ptr<T>`-returning algorithms) have no constructor symbol either; the factory function itself is what gets scored, under "functions," not "classes." This is why e.g. `bgsegm`'s background subtractors show `0/1` classes but `5/9` functions — the classes *are* usable from JS via their `create*` functions.
- Plain (non-constructor) member functions of a class that does export a constructor are not scored individually — only whether *any* constructor symbol was pulled in.

So the overall 12.4% is a reasonable proxy for "fraction of the OpenCV C++ surface reachable from JS," but the per-module 0% entries for `ml`/`stitching`/`text`/etc. are the reliable signal — the modules genuinely have no binding — while a stated 17-20% on `core`/`calib3d` undercounts what's actually exposed once you account for the header-only types above.

## Repository layout

See `CLAUDE.md` for the full architecture map: one `js_<concept>.cpp` per OpenCV class or module, wired together in `src/init_module.cpp`, with shared JS↔C++ glue in `include/jsbindings.hpp`. `js/*.js` are ES6 wrappers (`Window`, `TextStyle`, `Pipeline`, `VideoSource`, `ImageSequence`) built on top of the raw bindings; `tests/*.js` are standalone scripts run individually with `qjs`, not a test suite.
