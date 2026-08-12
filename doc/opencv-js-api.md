# opencv.js API Reference

Complete API surface of the official Emscripten/embind-based `opencv.js` bindings, compiled directly from the OpenCV source that generates them — not from tutorial pages, which only cover a subset.

**Sources** (local checkout at `/mnt/data/Projects/opencv`, OpenCV `5.0.0`, core module set only — no `opencv_contrib`):

- `platforms/js/opencv_js.config.py` — the whitelist of free functions and class methods exposed to JS. `modules/js/generator/embindgen.py` walks the C++ headers of every module compiled in and emits an `embind` binding *only* for symbols listed here (plus operator overloads implied by them). This is the authoritative list of what's callable from JS — anything not in this file does not exist in `opencv.js`, no matter how complete the underlying C++ module is.
- `modules/js/src/core_bindings.cpp` — hand-written `EMSCRIPTEN_BINDINGS` block. `cv::Mat` and the other value types (`Point`, `Size`, `Rect`, `Scalar`, ...) aren't produced by the generator — they're bound here directly, since their JS-friendly shape (flat arrays for `Scalar`, plain `{x,y}` objects for `Point`, typed-array views for pixel data) doesn't map 1:1 onto the C++ class.
- `modules/js/src/helpers.js` — pure-JS layer injected around the compiled wasm module: `imread`/`imshow`/`VideoCapture` (canvas/DOM glue), `matFromArray`, `matFromImageData`, and JS-side constructor sugar for `Point`/`Size`/`Rect`/`Scalar`/etc. so they behave like normal JS objects instead of embind value-objects.

This local checkout carries a couple of small patches against upstream `opencv/opencv@4.x` (a factory-method `Ptr<T>` namespace-qualification fix in `embindgen.py`, and `features2d` renamed to `features` / `calib3d` split into `_3d` + `calib`) — noted inline where they affect the whitelist shape below.

## Contents

- [Loading the module](#loading-the-module)
- [Value types](#value-types)
- [The `Mat` class](#the-mat-class)
- [Vector container types](#vector-container-types)
- [Type/depth constants](#typedepth-constants)
- [core](#core)
- [imgproc](#imgproc)
- [objdetect](#objdetect)
- [video](#video)
- [dnn](#dnn)
- [features (features2d)](#features-features2d)
- [photo](#photo)
- [_3d / calib (calib3d)](#_3d--calib-calib3d)
- [Browser/DOM helper layer](#browserdom-helper-layer)
- [What's *not* in opencv.js](#whats-not-in-opencvjs)
- [Memory management](#memory-management)

---

## Loading the module

```html
<script async src="opencv.js" onload="onOpenCvReady();"></script>
<script>
function onOpenCvReady() {
  cv['onRuntimeInitialized'] = () => {
    // cv.* is ready to use here
  };
}
</script>
```

`Module` (the raw Emscripten object) and `cv` are the same object once initialized — `helpers.js` does `if (typeof cv === 'undefined') { var cv = Module; }`.

---

## Value types

Defined twice, deliberately: an embind `value_object`/`value_array` in `core_bindings.cpp` (the wasm-side shape) and a plain-JS constructor in `helpers.js` (what you actually construct/pass — embind's `value_object` marshals to/from these transparently).

| Type | Shape | Notes |
|---|---|---|
| `Point(x, y)` | `{x, y}` | also `Point2f`, `Point3f` bound as value_objects for wasm interop |
| `Size(width, height)` | `{width, height}` | also `Size2f` |
| `Rect(...)` | `{x, y, width, height}` | overloads: `()`, `(rect)`, `(point, size)`, `(x, y, width, height)`. Also `Rect2f`, `Rect2d` |
| `RotatedRect(center, size, angle)` | `{center, size, angle}` | static helpers: `RotatedRect.points(rr)`, `.boundingRect(rr)`, `.boundingRect2f(rr)` — thin wrappers over `cv::RotatedRect::points()`/`boundingRect()`/`boundingRect2f()` |
| `Scalar(v0, v1, v2, v3)` | subclasses `Array` (so `scalar[0]`, `.length === 4`, etc.) | `Scalar.all(v)` → `Scalar(v,v,v,v)` |
| `Range(start, end)` | `{start, end}` | |
| `TermCriteria(type, maxCount, epsilon)` | `{type, maxCount, epsilon}` | |
| `MinMaxLoc(minVal, maxVal, minLoc, maxLoc)` | `{minVal, maxVal, minLoc, maxLoc}` | return type of `cv.minMaxLoc()` |
| `Circle(center, radius)` | `{center, radius}` | return type of `cv.minEnclosingCircle()` |
| `KeyPoint` | `{angle, class_id, octave, pt, response, size}` | value_object only, no JS constructor sugar |
| `DMatch` | `{queryIdx, trainIdx, imgIdx, distance}` | value_object only |
| `Moments` | `{m00, m10, m01, m20, m11, m02, m30, m21, m12, m03, mu20, mu11, mu02, mu30, mu21, mu12, mu03, nu20, nu11, nu02, nu30, nu21, nu12, nu03}` | return type of `cv.moments()`, value_object only |
| `Exception` | `{code, msg}` | thrown on C++-side `cv::Exception`; caught JS-side as this shape |

---

## The `Mat` class

Hand-bound in `core_bindings.cpp`; the single most important type in the API.

### Constructors

```js
new cv.Mat()
new cv.Mat(other)                      // copy handle (shallow — see clone())
new cv.Mat(size, type)                 // cv.Size, no data (uninitialized)
new cv.Mat(rows, cols, type)           // no data (uninitialized)
new cv.Mat(rows, cols, type, scalar)   // filled with scalar
new cv.Mat(rows, cols, type, dataPtr, step)  // raw pointer + step — internal use (binding_utils::createMat), not something JS code calls directly
```

There is **no constructor that takes a flat JS array or TypedArray directly** — shape must always be explicit. That's what `cv.matFromArray(rows, cols, type, array)` (a JS-level helper, not a `Mat` constructor overload) exists for — see [Browser/DOM helper layer](#browserdom-helper-layer). This is architecturally the same shape as this project's `new Mat(rows, cols, type, typedArray.buffer)`.

### Class (static) functions

```js
cv.Mat.eye(size, type)          cv.Mat.eye(rows, cols, type)
cv.Mat.ones(size, type)         cv.Mat.ones(rows, cols, type)
cv.Mat.zeros(size, type)        cv.Mat.zeros(rows, cols, type)
```

### Properties

| Property | Type | Notes |
|---|---|---|
| `.rows` | `int` | |
| `.cols` | `int` | |
| `.matSize` | `int[]` | one entry per dimension (`mat.dims` long) — via `getMatSize` |
| `.step` | `int[]` | byte stride per dimension — via `getMatStep` |
| `.data` | `Uint8Array` | zero-copy view into the underlying buffer |
| `.data8S` | `Int8Array` | |
| `.data16U` | `Uint16Array` | |
| `.data16S` | `Int16Array` | |
| `.data32S` | `Int32Array` | |
| `.data32F` | `Float32Array` | |
| `.data64F` | `Float64Array` | |

Each `.dataXX` property is a fresh `memory_view` computed on access — same underlying memory, reinterpreted per element type, length `= (total * elemSize) / sizeof(T)`. There is no `data8U`; plain `.data` is the `uint8` view.

### Methods

```js
mat.elemSize()                          mat.elemSize1()
mat.channels()                          mat.depth()
mat.type()                              mat.empty()
mat.total()                             mat.isContinuous()
mat.size()                              // -> Size (via matSize helper, distinct from .matSize property)

mat.convertTo(dst, rtype)
mat.convertTo(dst, rtype, alpha)
mat.convertTo(dst, rtype, alpha, beta)

mat.create(rows, cols, type)            mat.create(size, type)

mat.row(i)                              mat.col(i)
mat.rowRange(start, end)                mat.rowRange(range)
mat.colRange(start, end)                mat.colRange(range)
mat.roi(rect)                           // operator()
mat.step1(i)

mat.copyTo(dst)                         mat.copyTo(dst, mask)
mat.setTo(scalar)                       mat.setTo(scalar, mask)

mat.clone()                             // deep copy (aliased over embind's shallow default — see note below)
mat.mat_clone()                         // the actual deep-copy binding of cv::Mat::clone()
mat.dot(other)
mat.mul(other, scale)
mat.inv(decompMethod)
mat.t()                                 // transpose
mat.diag(d)                             mat.diag()

// raw pointer access -> typed-array view starting at (i[, j])
mat.ptr(i)            mat.ptr(i, j)          // Uint8Array
mat.ucharPtr(i)        mat.ucharPtr(i, j)     // Uint8Array
mat.charPtr(i)         mat.charPtr(i, j)      // Int8Array
mat.shortPtr(i)        mat.shortPtr(i, j)     // Int16Array
mat.ushortPtr(i)       mat.ushortPtr(i, j)    // Uint16Array
mat.intPtr(i)          mat.intPtr(i, j)       // Int32Array
mat.floatPtr(i)        mat.floatPtr(i, j)     // Float32Array
mat.doublePtr(i)       mat.doublePtr(i, j)    // Float64Array

// single-element access, all overloaded for 1D/2D/3D coordinates
mat.charAt(i)      mat.charAt(i, j)      mat.charAt(i, j, k)
mat.ucharAt(i)     mat.ucharAt(i, j)     mat.ucharAt(i, j, k)
mat.shortAt(i)     mat.shortAt(i, j)     mat.shortAt(i, j, k)
mat.ushortAt(i)    mat.ushortAt(i, j)    mat.ushortAt(i, j, k)
mat.intAt(i)       mat.intAt(i, j)       mat.intAt(i, j, k)
mat.floatAt(i)     mat.floatAt(i, j)     mat.floatAt(i, j, k)
mat.doubleAt(i)    mat.doubleAt(i, j)    mat.doubleAt(i, j, k)
```

Notable design choices, relevant when comparing against this project's binding style:

- **No generic `.at(row, col)`.** opencv.js instead exposes one `<type>At()` method per element type (`intAt`, `floatAt`, `doubleAt`, ...) — the caller states the element type explicitly rather than it being inferred from `mat.type()`. This sidesteps any "reported shape says X but actual layout is Y" class of bug entirely, since there's no shape inference happening at the accessor at all.
- **`clone()` deep-copy shadowing is a recent patch, not original design.** Emscripten 3.1.71+ added a default shallow `ClassHandle.clone()` on *every* bound class, which collided with `cv::Mat::clone()`'s deep-copy semantics ([opencv/opencv#26643](https://github.com/opencv/opencv/pull/26643), [#27572](https://github.com/opencv/opencv/issues/27572)). The fix in `helpers.js` binds the real deep copy as `mat_clone` in C++, then reassigns `cv.Mat.prototype.clone = cv.Mat.prototype.mat_clone` at `onRuntimeInitialized` time to restore the expected deep-copy behavior. Worth knowing if you ever see version-dependent `clone()` behavior discussions upstream.

---

## Vector container types

`register_vector<T>(name)` — thin embind wrappers over `std::vector<T>`, each gets `.size()`, `.get(i)`, `.push_back(v)`, `.set(i, v)` (and are iterable).

```
IntVector            (std::vector<int>)
CharVector           (std::vector<char>)
FloatVector          (std::vector<float>)
DoubleVector         (std::vector<double>)
StringVector         (std::vector<std::string>)
PointVector          (std::vector<cv::Point>)
Point2fVector        (std::vector<cv::Point2f>)
Point3fVector        (std::vector<cv::Point3_<float>>)
MatVector            (std::vector<cv::Mat>)
RectVector           (std::vector<cv::Rect>)
KeyPointVector       (std::vector<cv::KeyPoint>)
DMatchVector         (std::vector<cv::DMatch>)
CharVectorVector     (std::vector<std::vector<char>>)
DMatchVectorVector   (std::vector<std::vector<cv::DMatch>>)
KeyPointVectorVector (std::vector<std::vector<cv::KeyPoint>>)
PointVectorVector    (std::vector<std::vector<cv::Point>>)
```

These are the types functions like `findContours` (→ `MatVector` of point-Mats) or `detectAndCompute` (→ `KeyPointVector`, `Mat` descriptors) actually return/accept — there's no bare-JS-array marshaling for these; you construct/consume a `cv.MatVector` etc. explicitly and `.delete()` it when done.

---

## Type/depth constants

```
CV_8UC1  CV_8UC2  CV_8UC3  CV_8UC4
CV_8SC1  CV_8SC2  CV_8SC3  CV_8SC4
CV_16UC1 CV_16UC2 CV_16UC3 CV_16UC4
CV_16SC1 CV_16SC2 CV_16SC3 CV_16SC4
CV_32SC1 CV_32SC2 CV_32SC3 CV_32SC4
CV_32FC1 CV_32FC2 CV_32FC3 CV_32FC4
CV_64FC1 CV_64FC2 CV_64FC3 CV_64FC4

CV_8U  CV_8S  CV_16U  CV_16S  CV_32S  CV_32F  CV_64F

INT_MIN  INT_MAX
```

Note what's *missing* relative to native OpenCV: no `CV_16F`/`CV_16BF` (bfloat16), no `CV_32U`/`CV_64U`/`CV_64S`/`CV_Bool` (the OpenCV 5 integer-depth additions this project added recently) — `core_bindings.cpp`'s constant block hasn't been updated for those. Everything else (color-conversion codes like `COLOR_BGR2GRAY`, morphology shapes, threshold types, border types, etc.) is auto-exposed by `embindgen.py` as a side effect of being referenced in a whitelisted function's default-argument or parameter type — there's no separate manually-curated constants list for those; they ride along with whichever whitelisted function uses them.

---

## core

Free functions (no receiver / static in `cv.*`):

```
absdiff, add, addWeighted, bitwise_and, bitwise_not, bitwise_or, bitwise_xor, cartToPolar,
compare, convertScaleAbs, copyMakeBorder, countNonZero, determinant, dft, divide, divSpectrums,
eigen, exp, flip, getOptimalDFTSize, gemm, hconcat, inRange, invert, kmeans, log, magnitude,
max, mean, meanStdDev, merge, min, minMaxLoc, mixChannels, multiply, norm, normalize,
perspectiveTransform, polarToCart, pow, randn, randu, reduce, repeat, rotate, setIdentity,
setRNGSeed, solve, solvePoly, split, sqrt, subtract, trace, transform, transpose, vconcat,
setLogLevel, getLogLevel, LUT
```

Classes:

- **`Algorithm`** — base class, no methods whitelisted directly (subclasses like `CLAHE`, `BackgroundSubtractorMOG2` add their own).

---

## imgproc

Free functions:

```
adaptiveThreshold, applyColorMap, approxPolyDP, approxPolyN, arcLength, arrowedLine,
bilateralFilter, blendLinear, blur, boundingRect, boxFilter, calcBackProject, calcHist,
Canny, circle, clipLine, compareHist, connectedComponents, connectedComponentsWithStats,
contourArea, convertMaps, convexHull, convexityDefects, cornerHarris, cornerMinEigenVal,
createCLAHE, createHanningWindow, createLineSegmentDetector, cvtColor, demosaicing, dilate,
distanceTransform, distanceTransformWithLabels, drawContours, drawMarker, ellipse,
ellipse2Poly, equalizeHist, erode, fillConvexPoly, fillPoly, filter2D, findContours,
findContoursLinkRuns, fitEllipse, fitEllipseAMS, fitEllipseDirect, fitLine, floodFill,
GaussianBlur, getAffineTransform, getFontScaleFromHeight, getPerspectiveTransform,
getRectSubPix, getRotationMatrix2D, getStructuringElement, goodFeaturesToTrack, grabCut,
HoughLines, HoughLinesP, HoughCircles, HuMoments, integral, integral2, intersectConvexConvex,
invertAffineTransform, isContourConvex, Laplacian, line, matchShapes, matchTemplate,
medianBlur, minAreaRect, minEnclosingCircle, minEnclosingTriangle, moments, morphologyEx,
pointPolygonTest, polylines, preCornerDetect, putText, pyrDown, pyrUp, rectangle, remap,
resize, rotatedRectangleIntersection, Scharr, sepFilter2D, Sobel, spatialGradient,
sqrBoxFilter, stackBlur, threshold, warpAffine, warpPerspective, warpPolar, watershed
```

Classes:

- **`CLAHE`**: `apply`, `collectGarbage`, `getClipLimit`, `getTilesGridSize`, `setClipLimit`, `setTilesGridSize`
- **`segmentation_IntelligentScissorsMB`**: `IntelligentScissorsMB` (ctor), `setWeights`, `setGradientMagnitudeMaxLimit`, `setEdgeFeatureZeroCrossingParameters`, `setEdgeFeatureCannyParameters`, `applyImage`, `applyImageFeatures`, `buildMap`, `getContour`

Plus, bound directly in `core_bindings.cpp` under `#ifdef HAVE_OPENCV_IMGPROC` rather than generated:

```
cv.minEnclosingCircle(points)                                    // -> Circle {center, radius}
cv.floodFill(img, mask, seedPoint, newVal, rectOut, loDiff, upDiff, connectivity)  // + 4 fewer-arg overloads
cv.morphologyDefaultBorderValue()
```

---

## objdetect

Free functions:

```
getPredefinedDictionary, extendDictionary, drawDetectedMarkers, generateImageMarker,
drawDetectedCornersCharuco, drawDetectedDiamonds
```

*(Upstream `4.x` additionally whitelists `groupRectangles`, and `HOGDescriptor`/`CascadeClassifier` classes — absent from this local checkout's config, i.e. not callable from this build's `opencv.js`.)*

Classes:

- **`GraphicalCodeDetector`**: `decode`, `detect`, `detectAndDecode`, `detectMulti`, `decodeMulti`, `detectAndDecodeMulti`
- **`QRCodeDetector`**: ctor + all of `GraphicalCodeDetector`'s + `decodeCurved`, `detectAndDecodeCurved`, `setEpsX`, `setEpsY`
- **`aruco_PredefinedDictionaryType`**: (enum only, no methods)
- **`aruco_Dictionary`**: `Dictionary` (ctor), `getDistanceToId`, `generateImageMarker`, `getByteListFromBits`, `getBitsFromByteList`
- **`aruco_Board`**: `Board`, `matchImagePoints`, `generateImage`
- **`aruco_GridBoard`**: `GridBoard`, `generateImage`, `getGridSize`, `getMarkerLength`, `getMarkerSeparation`, `matchImagePoints`
- **`aruco_CharucoParameters`**: `CharucoParameters` (ctor)
- **`aruco_CharucoBoard`**: `CharucoBoard`, `generateImage`, `getChessboardCorners`, `getNearestMarkerCorners`, `checkCharucoCornersCollinear`, `matchImagePoints`, `getLegacyPattern`, `setLegacyPattern`
- **`aruco_DetectorParameters`**: `DetectorParameters` (ctor)
- **`aruco_RefineParameters`**: `RefineParameters` (ctor)
- **`aruco_ArucoDetector`**: `ArucoDetector`, `detectMarkers`, `refineDetectedMarkers`, `setDictionary`, `setDetectorParameters`, `setRefineParameters`
- **`aruco_CharucoDetector`**: `CharucoDetector`, `setBoard`, `setCharucoParameters`, `setDetectorParameters`, `setRefineParameters`, `detectBoard`, `detectDiamonds`
- **`QRCodeDetectorAruco_Params`**: `Params` (ctor)
- **`QRCodeDetectorAruco`**: ctor + `decode`, `detect`, `detectAndDecode`, `detectMulti`, `decodeMulti`, `detectAndDecodeMulti`, `setDetectorParameters`, `setArucoParameters`
- **`barcode_BarcodeDetector`**: ctor + `decode`, `detect`, `detectAndDecode`, `detectMulti`, `decodeMulti`, `detectAndDecodeMulti`, `decodeWithType`, `detectAndDecodeWithType`
- **`mcc_CheckerDetector`**: `process`, `getBestColorChecker`, `getListColorChecker`, `create`, `draw`, `getRefColors`, `setDetectionParams`, `getDetectionParams`, `setColorChartType`, `getColorChartType`, `setUseDnnModel`, `getUseDnnModel` *(local-checkout-only, `mcc` module)*
- **`mcc_DetectorParameters`**: `DetectorParametersMCC` *(local-checkout-only)*
- **`mcc_Checker`**: `setTarget`, `setBox`, `setChartsRGB`, `setChartsYCbCr`, `setCost`, `setCenter`, `getTarget`, `getBox`, `getColorCharts`, `getChartsRGB`, `getChartsYCbCr`, `getCost`, `getCenter` *(local-checkout-only)*
- **`FaceDetectorYN`**: `setInputSize`, `getInputSize`, `setScoreThreshold`, `getScoreThreshold`, `setNMSThreshold`, `getNMSThreshold`, `setTopK`, `getTopK`, `detect`, `create`

---

## video

Free functions:

```
CamShift, calcOpticalFlowFarneback, calcOpticalFlowPyrLK, createBackgroundSubtractorMOG2,
findTransformECC, meanShift
```

Classes:

- **`BackgroundSubtractorMOG2`**: ctor, `apply`
- **`BackgroundSubtractor`**: `apply`, `getBackgroundImage`
- **`TrackerMIL`**: `create`
- **`TrackerMIL_Params`**: (no methods whitelisted)

`Tracker.init`/`.update` are explicitly *not* whitelisted (`# issue #21070`) but still bound directly in `core_bindings.cpp` as pure-virtual wrapper functions:

```js
cv.CamShift(mat, rect, termCriteria)   // -> [RotatedRect, updatedRect]  (array, via emscripten::val)
cv.meanShift(mat, rect, termCriteria)  // -> [n, updatedRect]
// cv.Tracker base class with init()/update() exists but has no concrete subclass exposed except TrackerMIL
```

---

## dnn

```
dnn_Net: setInput, forward, setPreferableBackend, getUnconnectedOutLayersNames
'': readNetFromTensorflow, readNetFromONNX, readNetFromTFLite, readNet, blobFromImage
```

*(Upstream `4.x` additionally whitelists `readNetFromCaffe`, `readNetFromTorch`, `readNetFromDarknet` — absent here.)*

---

## features (features2d)

*(Named `features2d` upstream; this local checkout renamed the module/whitelist key to `features`, and correspondingly `HAVE_OPENCV_FEATURES` instead of `HAVE_OPENCV_FEATURES2D` in `core_bindings.cpp`.)*

- **`Feature2D`**: `detect`, `compute`, `detectAndCompute`, `descriptorSize`, `descriptorType`, `defaultNorm`, `empty`, `getDefaultName`
- **`ORB`**: `create`, `setMaxFeatures`, `setScaleFactor`, `setNLevels`, `setEdgeThreshold`, `setFastThreshold`, `setFirstLevel`, `setWTA_K`, `setScoreType`, `setPatchSize`, `getFastThreshold`, `getDefaultName`
- **`MSER`**: `create`, `detectRegions`, `setDelta`, `getDelta`, `setMinArea`, `getMinArea`, `setMaxArea`, `getMaxArea`, `setPass2Only`, `getPass2Only`, `getDefaultName`
- **`FastFeatureDetector`**: `create`, `setThreshold`, `getThreshold`, `setNonmaxSuppression`, `getNonmaxSuppression`, `setType`, `getType`, `getDefaultName`
- **`GFTTDetector`**: `create`, `setMaxFeatures`, `getMaxFeatures`, `setQualityLevel`, `getQualityLevel`, `setMinDistance`, `getMinDistance`, `setBlockSize`, `getBlockSize`, `setHarrisDetector`, `getHarrisDetector`, `setK`, `getK`, `getDefaultName`
- **`SimpleBlobDetector`**: `create`, `setParams`, `getParams`, `getDefaultName`
- **`SimpleBlobDetector_Params`**: (no methods whitelisted)
- **`DescriptorMatcher`**: `add`, `clear`, `empty`, `isMaskSupported`, `train`, `match`, `knnMatch`, `radiusMatch`, `clone`, `create`
- **`BFMatcher`**: `isMaskSupported`, `create`
- Free functions: `drawKeypoints`, `drawMatches`, `drawMatchesKnn`

*(Upstream `4.x` additionally whitelists `BRISK`, `AgastFeatureDetector`, `KAZE`, `AKAZE` — absent here.)*

---

## photo

Free functions:

```
createAlignMTB, createCalibrateDebevec, createCalibrateRobertson, createMergeDebevec,
createMergeMertens, createMergeRobertson, createTonemapDrago, createTonemapMantiuk,
createTonemapReinhard, inpaint
```

Classes:

- **`CalibrateCRF`**: `process`
- **`AlignExposures`**: `process`
- **`AlignMTB`**: `calculateShift`, `shiftMat`, `computeBitmaps`, `getMaxBits`, `setMaxBits`, `getExcludeRange`, `setExcludeRange`, `getCut`, `setCut`
- **`CalibrateDebevec`**: `getLambda`, `setLambda`, `getSamples`, `setSamples`, `getRandom`, `setRandom`
- **`CalibrateRobertson`**: `getMaxIter`, `setMaxIter`, `getThreshold`, `setThreshold`, `getRadiance`
- **`MergeExposures`**: `process`
- **`MergeDebevec`**: `process`
- **`MergeMertens`**: `process`, `getContrastWeight`, `setContrastWeight`, `getSaturationWeight`, `setSaturationWeight`, `getExposureWeight`, `setExposureWeight`
- **`MergeRobertson`**: `process`
- **`Tonemap`**: `process`, `getGamma`, `setGamma`
- **`TonemapDrago`**: `getSaturation`, `setSaturation`, `getBias`, `setBias`, `getSigmaColor`, `setSigmaColor`, `getSigmaSpace`, `setSigmaSpace`
- **`TonemapMantiuk`**: `getScale`, `setScale`, `getSaturation`, `setSaturation`
- **`TonemapReinhard`**: `getIntensity`, `setIntensity`, `getLightAdaptation`, `setLightAdaptation`, `getColorAdaptation`, `setColorAdaptation`

Notably absent: `pencilSketch`, `stylization`, `detailEnhance`, `edgePreservingFilter`, `fastNlMeansDenoising(Colored)` — the NPR filters this project bound in `js_photo.cpp` have no opencv.js equivalent at all.

---

## _3d / calib (calib3d)

*(Upstream `4.x` has a single `calib3d` whitelist key; this local checkout splits it into `_3d` (the `cv::` namespace calib functions) and `calib` (the `cv::fisheye` namespace + `UsacParams`), matching an upstream OpenCV 5.x module reorganization where `calib3d` was split into `3d` and `calib`.)*

`_3d` free functions:

```
findHomography, calibrateCameraExtended, drawFrameAxes, estimateAffine2D,
getDefaultNewCameraMatrix, initUndistortRectifyMap, Rodrigues, solvePnP, solvePnPRansac,
solvePnPRefineLM, projectPoints, undistort
```

`calib` free functions:

```
fisheye_initUndistortRectifyMap, fisheye_projectPoints
```

Classes:

- **`UsacParams`**: ctor only

---

## Browser/DOM helper layer

Pure JS, `modules/js/src/helpers.js`, injected at `Module['onRuntimeInitialized']` time — these don't exist in the wasm module itself:

```js
cv.imread(canvasOrImgElementOrId)        // -> Mat, CV_8UC4, via canvas getImageData()
cv.imshow(canvasElementOrId, mat)        // draws a Mat to a <canvas>, converting to CV_8UC4 first
cv.VideoCapture(videoElementOrId)        // .read(frameMat) pulls the current video frame via canvas
cv.matFromArray(rows, cols, type, array) // builds a Mat and .set()s the right typed .dataXX view
cv.matFromImageData(imageData)           // -> Mat CV_8UC4 from a DOM ImageData
```

These all require a DOM (`document`, `HTMLCanvasElement`, `HTMLVideoElement`, `ImageData`) — none of them are usable outside a browser/browser-like environment (e.g. not in this project's `qjs` REPL context, which has none of those globals).

Also added here, not in `core_bindings.cpp`:

- `cv.Mat.prototype[Symbol.dispose] = cv.Mat.prototype.delete` (and same for `cv.UMat`) — TC39 `using` declaration support (TypeScript 5.2+ / future JS), guarded by `typeof Symbol.dispose !== 'undefined'`.
- The `clone()` deep-copy reassignment discussed under [The Mat class](#the-mat-class).

---

## What's *not* in opencv.js

Given only `core, imgproc, objdetect, video, dnn, features, photo, _3d, calib` are whitelisted at all, and only from modules actually compiled into this build (`core, dnn, features, imgproc, objdetect, photo, video`, plus `_3d`/`calib` — no `ximgproc`, no `xphoto`, no `stitching` despite those modules existing in this checkout's `modules/` directory), everything else is absent from `opencv.js` no matter how complete the C++ side is:

- **`imgcodecs`** (`cv::imread`/`cv::imwrite` proper) — the browser build uses the DOM/canvas route (`cv.imread`/`cv.imshow`) instead, never the native codec path.
- **`videoio`** (`cv::VideoCapture`/`cv::VideoWriter`) — same story, DOM `<video>` element instead.
- **`ximgproc`, `xphoto`** — not compiled in; none of `guidedFilter`, `l0Smooth`, `oilPainting`, `bm3dDenoising`, etc. exist in opencv.js at all, regardless of contrib availability.
- **`calib3d`'s full surface** — only 12 of the module's functions are whitelisted; `calibrateCamera` (non-extended), `triangulatePoints`, stereo-rectification functions, etc. are absent.
- **`stitching`, `photo`'s NPR filters, `ml`, `text`, `saliency`, ...** — not whitelisted, not reachable, independent of whether the underlying module is compiled.

This is architecturally different from this project's `qjs-opencv`, which binds close to the full `cv::` namespace it cares about (see this repo's own `TODO.md` coverage survey) rather than a hand-curated browser-oriented subset.

---

## Memory management

Every embind-wrapped C++ object (`Mat`, `MatVector`, `CLAHE`, detector/matcher instances, ...) must be freed explicitly with `.delete()` — there's no finalizer-driven cleanup the way this project's `qjs-opencv` bindings work (which rely on QuickJS GC + C++ destructors in the class finalizer). Forgetting `.delete()` in opencv.js is a real, common leak; this project's "no copies, mutable, finalizers do the work" memory model (see this repo's own `README`/`CLAUDE.md`) is a deliberate design difference, not an oversight.
