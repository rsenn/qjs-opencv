# Fisheye cameras and camera calibration

Calibrate once, then undistort every frame. All functions use the opencv.js signatures (outputs are arguments, per-view
lists are JS arrays of `cv.Mat`). Tests: `tests/test_fisheye.js`; example: `examples/fisheye-undistort.js`.

## Workflow

```js
import * as cv from 'opencv';

/* 1. per view: find the corners, refine them, keep object and image points */
const found = cv.findChessboardCorners(gray, [9, 6], corners);
cv.cornerSubPix(gray, corners, [3, 3], [-1, -1], [3, 30, 0.01]);   // [type, maxCount, epsilon]; 3 = EPS + COUNT

/* 2. calibrate: objectPoints/imagePoints are arrays of CV_64FC3 / CV_64FC2 Mat, one per view */
const rms = cv.fisheye.calibrate(objectPoints, imagePoints, [w, h], K, D, rvecs, tvecs,
                                 cv.fisheye.CALIB_RECOMPUTE_EXTRINSIC | cv.fisheye.CALIB_FIX_SKEW);

/* 3. maps, once */
cv.fisheye.estimateNewCameraMatrixForUndistortRectify(K, D, [w, h], R, Knew, balance);   // R = identity
cv.fisheye.initUndistortRectifyMap(K, D, R, Knew, [w, h], cv.CV_16SC2, map1, map2);

/* 4. per frame */
cv.remap(frame, out, map1, map2, cv.INTER_LINEAR, cv.BORDER_CONSTANT, new cv.Scalar(0, 0, 0, 0));
```

`cv.fisheye.undistortImage(src, dst, K, D, Knew)` does steps 3 and 4 in one call: fine for a single picture, wasteful per
frame. With K and D known (a datasheet or an earlier calibration) skip step 1 and 2.

The pinhole lens model has the same pieces at top level: `cv.undistort`, `cv.initUndistortRectifyMap`,
`cv.getOptimalNewCameraMatrix`, `cv.getDefaultNewCameraMatrix`, `cv.undistortPoints`, `cv.undistortImagePoints`,
`cv.projectPoints`, `cv.solvePnP`, `cv.solvePnPRansac`, `cv.solvePnPGeneric`, `cv.Rodrigues`, `cv.drawFrameAxes`,
`cv.findCirclesGrid`, `cv.initCameraMatrix2D`, `cv.calibrateCamera`, `cv.calibrateCameraRO`, `cv.stereoCalibrate`, with the
`CALIB_*`, `SOLVEPNP_*`, `RANSAC` and `LMEDS` constants. The fisheye flags stay under `cv.fisheye`.

## OpenCV 5 and point list shapes

OpenCV 5's calibration functions take each view's points as one row (1xN); 4.x accepted Nx1 too. The binding reshapes Nx1 to
1xN (no copy) for the list arguments, so Mats from `findChessboardCorners` or `projectPoints` work as they are.
`cv.fisheye.calibrate` needs `CV_64F`/`CV_32F` 3-channel object points and 2-channel image points of one depth.
`cv.fisheye.projectPoints(objectPoints, imagePoints, rvec, tvec, K, D, alpha, jacobian)` also accepts a `cv.Affine3` in place
of `rvec, tvec`.

## Speed

The maps are the expensive part and are computed once. Per frame, `cv.remap` is one bilinear lookup per output pixel, memory
bound. On the x86_64 host a 640x480 BGR frame costs well under 3 ms (`examples/fisheye-undistort.js undistort ... FRAMES` prints
the rate: 390 fps including reading PNGs). The Raspberry Pi has not been measured yet; to keep it cheap use `CV_16SC2` maps
(not `CV_32FC1`), undistort at the capture resolution (not larger), and undistort a gray frame when the pipeline allows it.

## Capturing on the Raspberry Pi

- CSI camera modules (libcamera): `cv.RaspiCam` (LCCV), present when the binding was built with `USE_LCCV` (the default `AUTO`
  enables it when libcamera is found): `new cv.RaspiCam()`, `cam.options.videoWidth`/`videoHeight`, `cam.startVideo()`,
  `cam.getVideoFrame(mat, timeoutMs)`. `examples/fisheye-undistort.js undistort calib.json raspicam` uses it.
- USB (UVC) cameras: `new cv.VideoCapture(index)` (V4L2). A CSI module is not a usable `/dev/video0` for OpenCV on current
  Raspberry Pi OS, so `VideoCapture(0)` does not reach it.

## Not bound yet (calib3d / geometry / imgproc)

Checked against the 5.1 headers: calibrateHandEye, calibrateRobotWorldHandEye, calibrateMultiview, calibrationMatrixValues,
composeRT, computeCorrespondEpilines, convertPointsFromHomogeneous/ToHomogeneous/Homogeneous, correctMatches,
decomposeEssentialMat, decomposeHomographyMat, decomposeProjectionMatrix, filterHomographyDecompByVisibleRefpoints,
filterSpeckles, findEssentialMat, findFundamentalMat, getUndistortRectangles, getValidDisparityROI, matMulDeriv, recoverPose,
rectify3Collinear, registerCameras, reprojectImageTo3D, RQDecomp3x3, sampsonDistance, solveP3P, solvePnPRefineLM,
solvePnPRefineVVS, stereoRectifyUncalibrated, triangulatePoints, validateDisparity, estimateTranslation2D/3D, approxPolyN,
minEnclosingConvexPolygon, buildMST, farthestPointSampling, voxelGridSampling, randomSampling, normalEstimate; imgproc:
arrowedLine, drawMarker, compareHist, cornerEigenValsAndVecs, cornerMinEigenVal, preCornerDetect, ellipse2Poly, EMD,
findContoursLinkRuns, getClosestEllipsePoints, HoughLinesPointSet, phaseCorrelateIterative, thresholdWithMask.
